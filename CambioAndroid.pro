
TARGET = cambio

CONFIG += mobility

MOBILITY =

QT += core gui widgets
android: QT += androidextras

TEMPLATE = app

DEFINES += SpecUtilsAsLib
android: DEFINES += ANDROID
ios: DEFINES += IOS

HEADERS += build_test/SpecUtils_config.h \
           SpecUtils/SpecUtils/UtilityFunctions.h \
           SpecUtils/SpecUtils/SpectrumDataStructs.h \
           cambio/TimeView.h \
           cambio/TimeChart.h \
           cambio/CambioApp.h \
           cambio/MainWindow.h \
           cambio/SaveWidget.h \
           cambio/SpectrumView.h \
           cambio/SpectrumChart.h \
           cambio/FileDetailWidget.h \
           cambio/BatchConvertDialog.h \
           cambio/BusyIndicator.h \
           cambio/sandia_logo.h

SOURCES +=  main.cpp \
           SpecUtils/src/UtilityFunctions.cpp \
           SpecUtils/src/SpectrumDataStructs.cpp \
           src/TimeView.cpp \
           src/TimeChart.cpp \
           src/CambioApp.cpp \
           src/MainWindow.cpp \
           src/SaveWidget.cpp \
           src/SpectrumView.cpp \
           src/SpectrumChart.cpp \
           src/FileDetailWidget.cpp \
           src/BatchConvertDialog.cpp \
           src/BusyIndicator.cpp \
           src/sandia_logo.cpp

android: SOURCES += target/android/android.cpp

INCLUDEPATH += /Users/wcjohns/rad_ana/cambiopp/cambio \
               /Users/wcjohns/rad_ana/cambiopp/cambio/SpecUtils

android: INCLUDEPATH += /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/include \
               /Users/wcjohns/rad_ana/cambiopp/cambio/build_test \
               /Users/wcjohns/install/Qt5.4/android_armv7/include/QtCommercialChart \
               /Users/wcjohns/install/Qt5.4/EnterpriseAddOns/Charts/2.0/Src/include/QtCharts

ios:  INCLUDEPATH += /Users/wcjohns/rad_ana/InterSpec/target/ios/prefix/include \
               /Users/wcjohns/rad_ana/cambiopp/cambio/build_ios \
               /Users/wcjohns/install/Qt5.4/ios/include/QtCommercialChart

android: LIBS +=  -L/Users/wcjohns/install/android/armstandalone15/sysroot/usr/lib \
         -L/Users/wcjohns/install/Qt5.4/android_armv7/lib \
         -L/Users/wcjohns/install/Qt5.4/android_armv7/plugins/platforms
android: LIBS += /Users/wcjohns/install/Qt5.4/5.4/android_armv7/lib/libQt5Charts.so \
         /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/lib/libboost_thread-gcc-mt-1_57.a \
         /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/lib/libboost_system-gcc-mt-1_57.a \
         /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/lib/libboost_filesystem-gcc-mt-1_57.a \
         /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/lib/libboost_date_time-gcc-mt-1_57.a \
         /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/lib/libboost_chrono-gcc-mt-1_57.a \
         /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/lib/libboost_serialization-gcc-mt-1_57.a \
         /Users/wcjohns/install/android/armstandalone19abi4.8/sysroot/usr/lib/libboost_atomic-gcc-mt-1_57.a \


#MOBILITY =
#system(cd /Users/wcjohns/rad_ana/cambiopp/cambio/build_test && make -j8)

android: ANDROID_PACKAGE_SOURCE_DIR = $$PWD/target/android

android: OTHER_FILES += \
    target/android/src/org/sandia/cambio/CambioActivity.java \
    target/android/AndroidManifest.xml
