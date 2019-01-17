
For Qt 5.2.1 on OS X, got Qt to compile with 
./configure -no-framework -no-icu -no-openssl -static -release -prefix /Users/wcjohns/install/Qt5.2.1_static -no-xcb-xlib -no-xcb --no-glib --no-dbus -no-compile-examples -nomake examples
(But had to mod a few things in the source)

For Qt 5.11.1 on OS X, got Qt to compile with 
export MACOSX_DEPLOYMENT_TARGET=10.11
./configure -no-framework -no-icu -no-openssl -static -release -prefix /Users/wcjohns/install/Qt5.11.1_static -no-xcb-xlib -no-xcb --no-glib --no-dbus -no-cups -no-compile-examples -nomake examples -opensource -confirm-license -no-framework --c++std=c++14



For Android I ended up having to make CambioAndroid.pro which is a hack and a half, but if you create a QT Creator project from this .pro file, it compiles and loads the app onto the device, however it relies on build_test/SpecUtils_config.h existing, and all libraries are tied very specifically to my mac.  I tried generating this .pro file from CMake, but ultimately couldn’t get it to work out.

To compile statically on linux, should look at using https://github.com/wheybags/glibc_version_header or use something like Alpine linux to compile things.


To compile Qt statically on windows, I had to use the open source license (the
commercial one wouldnt finish configuring for some reason), and change the 
MD to MT flags in the mkspecs/win32-msvc2012/qmake.conf file (to static link
to the VC runtime), and then execute, using the standard windows command prompt:

"C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x86
configure -no-openssl -no-icu -static -release -nomake examples -platform win32-msvc2012 -no-opengl -opensource -prefix C:\Qt5.3_static_min
nmake
nmake install
(I also set a few environemnt variables according to http://qt-project.org/doc/qt-5/windows-building.html, but I'm not actually sure they were necassarry)
Also, the Qt source code must be on the C drive, and not a networked drive.

I then cd'd to, and added
CONFIG += static 
to the Charts pacakge and did a
C:\Qt5.3_static_min\bin\qmake.exe
nmake
And then just manually copied over the resulting includes, libraries and plugins to the Qt install directory.

Then into the VS project, I added the following linkings (I'm sure not all of 
them are necassary), and this produced an executable that didnt have any 
external dependancies (11,082 kb, compressed to ~5 MB; without opengl 10,372 kb
exe):
