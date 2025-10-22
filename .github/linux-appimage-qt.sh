if [[ -z $GITHUB_WORKSPACE ]]; then
	GITHUB_WORKSPACE="${PWD%/*}"
fi

export Qt6_DIR="/usr/lib/qt6"
export PATH="$Qt6_DIR/bin:$PATH"
export EXTRA_QT_PLUGINS="waylandcompositor"
export EXTRA_PLATFORM_PLUGINS="libqwayland-egl.so;libqwayland-generic.so"

# Prepare Tools for building the AppImage
wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-checkrt/releases/download/continuous/linuxdeploy-plugin-checkrt-x86_64.sh

chmod a+x linuxdeploy-x86_64.AppImage
chmod a+x linuxdeploy-plugin-qt-x86_64.AppImage
chmod a+x linuxdeploy-plugin-checkrt-x86_64.sh

# Build AppImage
./linuxdeploy-x86_64.AppImage --appdir AppDir
./linuxdeploy-plugin-checkrt-x86_64.sh --appdir AppDir

./linuxdeploy-x86_64.AppImage --appdir AppDir -d "$GITHUB_WORKSPACE"/PKGInstall.desktop  -e "$GITHUB_WORKSPACE"/build/PKGInstall -i "$GITHUB_WORKSPACE"/PKGIcon.png --plugin qt
./linuxdeploy-x86_64.AppImage --appdir AppDir --output appimage
mv PKGInstall-x86_64.AppImage PKGInstall.AppImage
