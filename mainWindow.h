#include <QMainWindow>
#include <filesystem>

#include "src/pkg.h"
#include "src/psf.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void pkgButtonClicked();
    void folderButtonClicked();
    void dlcButtonClicked();

private:
    std::optional<std::filesystem::path> FindGameByID(const std::filesystem::path &dir,
                                                      const std::string &game_id,
                                                      int max_depth);
    void InstallDragDropPkg(std::filesystem::path file);
    void LoadSettings();
    void SaveSettings();
    void LoadFoldersFromShadps4File();
    void GetSettingsFileLocation();

    Ui::MainWindow* ui;

    bool useSeparateUpdate = true;
    std::filesystem::path outputPath = "";
    std::filesystem::path dlcPath = "";
    std::filesystem::path pkgPath = "";
    std::filesystem::path tomlPath = "";
    std::filesystem::path settingsFile;
    PKG pkg;
    PSF psf;

    // static funcs
    std::filesystem::path PathFromQString(const QString& path) {
#ifdef _WIN32
        return std::filesystem::path(path.toStdWString());
#else
        return std::filesystem::path(path.toStdString());
#endif
    }

    void PathToQString(QString& result, const std::filesystem::path& path) {
#ifdef _WIN32
        result = QString::fromStdWString(path.wstring());
#else
        result = QString::fromStdString(path.string());
#endif
    }

    std::vector<std::string> SplitString(const std::string& str, char delimiter) {
        std::istringstream iss(str);
        std::vector<std::string> output(1);

        while (std::getline(iss, *output.rbegin(), delimiter)) {
            output.emplace_back();
        }

        output.pop_back();
        return output;
    }

    std::string PathToUTF8String(const std::filesystem::path& path) {
        const auto u8_string = path.u8string();
        return std::string{u8_string.begin(), u8_string.end()};
    }
};
