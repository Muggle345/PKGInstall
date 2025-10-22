#include <QFileDialog>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStyle>
#include <QtConcurrent/QtConcurrentMap>

#include "./ui_mainWindow.h"
#include "mainWindow.h"
#include "src/loader.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setWindowTitle("PKGInstall");

    connect(ui->browsePkgButton, &QPushButton::clicked, this, &MainWindow::pkgButtonClicked);
    connect(ui->browseFolderButton, &QPushButton::clicked, this, &MainWindow::folderButtonClicked);
    connect(ui->dlcFolderButton, &QPushButton::clicked, this, &MainWindow::dlcButtonClicked);
    connect(ui->closeButton, &QPushButton::clicked, this, &MainWindow::close);

    connect(ui->extractButton, &QPushButton::clicked, this, [this]() {
        InstallDragDropPkg(pkgPath);
    });
}

void MainWindow::folderButtonClicked() {
    QString folder = QFileDialog::getExistingDirectory(nullptr,
                                                       "Set Output folder",
                                                       QDir::homePath());

    ui->outputLineEdit->setText(folder);
    outputPath = PathFromQString(folder);
}

void MainWindow::dlcButtonClicked() {
    QString folder = QFileDialog::getExistingDirectory(nullptr, "Set DLC folder", QDir::homePath());

    ui->dlcLineEdit->setText(folder);
    dlcPath = PathFromQString(folder);
}

void MainWindow::pkgButtonClicked() {
    QString file = QFileDialog::getOpenFileName(nullptr,
                                                "Set Output folder",
                                                QDir::homePath(),
                                                "PKGs (*.pkg)");

    pkgPath = PathFromQString(file);
    ui->pkgLineEdit->setText(file);
}

void MainWindow::InstallDragDropPkg(std::filesystem::path file) {

    if (pkgPath == "" || outputPath == "") {
        QMessageBox::information(this, "Error", "PKG file and output folder must be set");
        return;
    }

    if (Loader::DetectFileType(file) == Loader::FileTypes::Pkg) {
        std::string failreason;
        pkg = PKG();
        if (!pkg.Open(file, failreason)) {
            QMessageBox::critical(nullptr, tr("PKG ERROR"), QString::fromStdString(failreason));
            return;
        }
        if (!psf.Open(pkg.sfo)) {
            QMessageBox::critical(nullptr,
                                  tr("PKG ERROR"),
                                  "Could not read SFO. Check log for details");
            return;
        }
        auto category = psf.GetString("CATEGORY");

        std::filesystem::path game_install_dir = outputPath;
        bool separateUpdate = true;                  // TODO

        QString pkgType = QString::fromStdString(pkg.GetPkgFlags());
        bool use_game_update = pkgType.contains("PATCH") && separateUpdate;

        // Default paths
        auto game_folder_path = game_install_dir / pkg.GetTitleID();
        auto game_update_path = use_game_update ? game_folder_path.parent_path()
                                                      / (std::string{pkg.GetTitleID()} + "-patch")
                                                : game_folder_path;
        const int max_depth = 5;

        if (pkgType.contains("PATCH")) {
            // For patches, try to find the game recursively
            auto found_game = FindGameByID(game_install_dir,
                                           std::string{pkg.GetTitleID()},
                                           max_depth);
            if (found_game.has_value()) {
                game_folder_path = found_game.value().parent_path();
                game_update_path = use_game_update
                                       ? game_folder_path.parent_path()
                                             / (std::string{pkg.GetTitleID()} + "-patch")
                                       : game_folder_path;
            }
        } else {
            // For base games, we check if the game is already installed
            auto found_game = FindGameByID(game_install_dir,
                                           std::string{pkg.GetTitleID()},
                                           max_depth);
            if (found_game.has_value()) {
                game_folder_path = found_game.value().parent_path();
            }
            // If the game is not found, we install it in the game install directory
            else {
                game_folder_path = game_install_dir / pkg.GetTitleID();
            }
            game_update_path = use_game_update ? game_folder_path.parent_path()
                                                     / (std::string{pkg.GetTitleID()} + "-patch")
                                               : game_folder_path;
        }

        QString gameDirPath;
        PathToQString(gameDirPath, game_folder_path);
        QDir game_dir(gameDirPath);
        if (game_dir.exists()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("PKG Installation"));

            std::string content_id;
            if (auto value = psf.GetString("CONTENT_ID"); value.has_value()) {
                content_id = std::string{*value};
            } else {
                QMessageBox::critical(this, tr("PKG ERROR"), "PSF file there is no CONTENT_ID");
                return;
            }
            std::string entitlement_label = SplitString(content_id, '-')[2];

            auto addon_extract_path = dlcPath;
            QString addonDirPath;
            PathToQString(addonDirPath, addon_extract_path);
            QDir addon_dir(addonDirPath);

            if (pkgType.contains("PATCH")) {
                QString pkg_app_version;
                if (auto app_ver = psf.GetString("APP_VER"); app_ver.has_value()) {
                    pkg_app_version = QString::fromStdString(std::string{*app_ver});
                } else {
                    QMessageBox::critical(this, tr("PKG ERROR"), "PSF file there is no APP_VER");
                    return;
                }
                std::filesystem::path sce_folder_path
                    = std::filesystem::exists(game_update_path / "sce_sys" / "param.sfo")
                          ? game_update_path / "sce_sys" / "param.sfo"
                          : game_folder_path / "sce_sys" / "param.sfo";
                psf.Open(sce_folder_path);
                QString game_app_version;
                if (auto app_ver = psf.GetString("APP_VER"); app_ver.has_value()) {
                    game_app_version = QString::fromStdString(std::string{*app_ver});
                } else {
                    QMessageBox::critical(this, tr("PKG ERROR"), "PSF file there is no APP_VER");
                    return;
                }
                double appD = game_app_version.toDouble();
                double pkgD = pkg_app_version.toDouble();
                if (pkgD == appD) {
                    msgBox.setText(QString(tr("Patch detected!") + "\n"
                                           + tr("PKG and Game versions match: ") + pkg_app_version
                                           + "\n" + tr("Would you like to overwrite?")));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                } else if (pkgD < appD) {
                    msgBox.setText(QString(
                        tr("Patch detected!") + "\n"
                        + tr("PKG Version %1 is older than existing version: ").arg(pkg_app_version)
                        + game_app_version + "\n" + tr("Would you like to overwrite?")));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                } else {
                    msgBox.setText(QString(
                        tr("Patch detected!") + "\n" + tr("Game exists: ") + game_app_version + "\n"
                        + tr("Would you like to apply Patch: ") + pkg_app_version + " ?"));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                }
                int result = msgBox.exec();
                if (result == QMessageBox::Yes) {
                    // Do nothing.
                } else {
                    return;
                }
            } else if (category == "ac") {
                if (!addon_dir.exists()) {
                    QMessageBox addonMsgBox;
                    addonMsgBox.setWindowTitle(tr("DLC Install"));
                    addonMsgBox.setText(QString(tr("Would you like to install DLC: %1?"))
                                            .arg(QString::fromStdString(entitlement_label)));

                    addonMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    addonMsgBox.setDefaultButton(QMessageBox::No);
                    int result = addonMsgBox.exec();
                    if (result == QMessageBox::Yes) {
                        game_update_path = addon_extract_path;
                    } else {
                        return;
                    }
                } else {
                    msgBox.setText(QString(tr("DLC already installed:") + "\n" + addonDirPath
                                           + "\n\n" + tr("Would you like to overwrite?")));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::No);
                    int result = msgBox.exec();
                    if (result == QMessageBox::Yes) {
                        game_update_path = addon_extract_path;
                    } else {
                        return;
                    }
                }
            } else {
                msgBox.setText(QString(tr("Game already installed") + "\n" + gameDirPath + "\n"
                                       + tr("Would you like to overwrite?")));
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                int result = msgBox.exec();
                if (result == QMessageBox::Yes) {
                    // Do nothing.
                } else {
                    return;
                }
            }
        } else {
            // Do nothing;
            if (pkgType.contains("PATCH") || category == "ac") {
                QMessageBox::information(
                    this,
                    tr("PKG Installation"),
                    tr("PKG is a patch or DLC, please install base game first!"));
                return;
            }
            // what else?
        }

        if (!pkg.Extract(file, game_update_path, failreason)) {
            QMessageBox::critical(this, tr("PKG ERROR"), QString::fromStdString(failreason));
        } else {
            int nfiles = pkg.GetNumberOfFiles();

            if (nfiles > 0) {
                QVector<int> indices;
                for (int i = 0; i < nfiles; i++) {
                    indices.append(i);
                }

                QProgressDialog dialog;
                dialog.setWindowTitle(tr("PKG Installation"));
                dialog.setWindowModality(Qt::WindowModal);
                QString extractmsg = QString(tr("Installing PKG"));
                dialog.setLabelText(extractmsg);
                dialog.setAutoClose(true);
                dialog.setRange(0, nfiles);

                dialog.setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                                       Qt::AlignCenter,
                                                       dialog.size(),
                                                       this->geometry()));

                QFutureWatcher<void> futureWatcher;
                connect(&futureWatcher, &QFutureWatcher<void>::finished, this, [=, this]() {
                    QString path;

                    // We want to show the parent path instead of the full path
                    PathToQString(path, game_folder_path.parent_path());
                    QIcon windowIcon(
                        PathToUTF8String(game_folder_path / "sce_sys/icon0.png").c_str());

                    QMessageBox extractMsgBox(this);
                    extractMsgBox.setWindowTitle(tr("Installation Finished"));
                    if (!windowIcon.isNull()) {
                        extractMsgBox.setWindowIcon(windowIcon);
                    }
                    extractMsgBox.setText(
                        QString(tr("Game successfully installed at %1")).arg(path));
                    extractMsgBox.addButton(QMessageBox::Ok);
                    extractMsgBox.setDefaultButton(QMessageBox::Ok);
                    connect(&extractMsgBox,
                            &QMessageBox::buttonClicked,
                            this,
                            [&](QAbstractButton *button) {
                                if (extractMsgBox.button(QMessageBox::Ok) == button) {
                                    extractMsgBox.close();
                                    // emit ExtractionFinished();
                                }
                            });
                    extractMsgBox.exec();

                    //if (delete_file_on_install) {
                    //  std::filesystem::remove(file);
                    //}
                });

                connect(&dialog, &QProgressDialog::canceled, [&]() { futureWatcher.cancel(); });

                connect(&futureWatcher,
                        &QFutureWatcher<void>::progressValueChanged,
                        &dialog,
                        &QProgressDialog::setValue);

                futureWatcher.setFuture(
                    QtConcurrent::map(indices, [&](int index) { pkg.ExtractFiles(index); }));

                dialog.exec();
            }
        }
    } else {
        QMessageBox::critical(this,
                              tr("PKG ERROR"),
                              tr("File doesn't appear to be a valid PKG file"));
    }
}

std::optional<std::filesystem::path> MainWindow::FindGameByID(const std::filesystem::path &dir,
                                                              const std::string &game_id,
                                                              int max_depth)
{
    if (max_depth < 0) {
        return std::nullopt;
    }

    // Check if this is the game we're looking for
    if (dir.filename() == game_id && std::filesystem::exists(dir / "sce_sys" / "param.sfo")) {
        auto eboot_path = dir / "eboot.bin";
        if (std::filesystem::exists(eboot_path)) {
            return eboot_path;
        }
    }

    // Recursively search subdirectories
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_directory()) {
            continue;
        }
        if (auto found = FindGameByID(entry.path(), game_id, max_depth - 1)) {
            return found;
        }
    }

    return std::nullopt;
}

MainWindow::~MainWindow()
{
    delete ui;
}
