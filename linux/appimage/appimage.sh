#!/bin/bash

curl -SL https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -o appimagetool

mkdir -p AppDir/usr/
rsync -av --exclude='*.o' --prune-empty-dirs linux/releasex64/ AppDir/usr/bin
cp linux/appimage/AppRun AppDir/
cp /usr/bin/yad AppDir/usr/bin
chmod +x AppDir/AppRun
chmod +x AppDir/usr/bin
ln -sr AppDir/usr/bin/quake2 AppDir/AppRun

cp linux/appimage/vkQuake2_512x512.png AppDir/vkquake.png
cp linux/appimage/vkquake.desktop AppDir/vkquake.desktop
mkdir -p AppDir/usr/share/applications && cp ./AppDir/vkquake.desktop ./AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons && cp ./AppDir/vkquake.png ./AppDir/usr/share/icons
mkdir -p AppDir/usr/share/icons/hicolor/512x512/apps && cp ./AppDir/vkquake.png ./AppDir/usr/share/icons/hicolor/512x512/apps
cp linux/appimage/icon.png AppDir/usr/share/icons/

mkdir -p assets/demo
curl -sSfL https://github.com/kondrak/vkQuake2/releases/download/1.5.9/vkquake2-1.5.9_win64.zip -o assets/vkquake2-1.5.9_win64.zip
cd assets
unzip -qq *.zip **/baseq2/players/* **/baseq2/pak0.pak -d .
cp -r **/baseq2/* demo
cd -
cp -r assets/demo AppDir/usr/bin/

chmod a+x appimagetool
./appimagetool AppDir/ vkQuake.AppImage
