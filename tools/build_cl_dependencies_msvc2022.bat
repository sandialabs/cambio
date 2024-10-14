@echo off
setlocal enableDelayedExpansion

rem This scripts builds the Boost and zlib dependencies for command line version of cambio, for Windows
rem This script is meant to be used from within the "x64 Native Tools Command Prompt for VS 2022" terminal
rem Usage: build_cl_dependencies_msvc2022.bat "Path to build at" "Path to install to"

IF "%2"=="" GOTO invalidarg
IF not "%3"=="" GOTO invalidarg

pushd .

rem By default we will only build the Release versions of the libraries, but
rem if you would like to build the Debug version (probably for developing 
rem cambio), then just set `builddebug` equal to something.
set "builddebug="

set ORIG_DIR=%CD%


if not exist "%1" (
  MKDIR "%1" && (
    echo "Created build dir %1"
  ) || (
    echo "Failed to make dir %1"
    popd
    exit /b -2
  )
) else (
    echo "Build directory %1 already exists."
)

pushd .
cd %1
set BUILD_DIR=%CD%
popd


if not exist "%2" (
  MKDIR "%2" && (
    echo "Created install dir %2"
  ) || (
    echo "Failed to make dir %2"
    popd
    exit /b -3
  )
) else (
    echo "Install directory %2 already exists."
)

if defined builddebug (
    echo "Building Debug and Release libraries."
) else (
    echo "Only building Release libraries, and not Debug."
)



pushd .
cd %2
set MY_PREFIX=%CD%
popd

echo "Will build in %BUILD_DIR%"
echo "Will install to %MY_PREFIX%"

cd %BUILD_DIR%

set errorlevel=

rem Build/install boost
set BOOST_TAR=boost_1_78_0.tar.gz
set BOOST_DIR=boost_1_78_0
set BOOST_BUILT_FILE=built_%BOOST_DIR%
set BOOST_REQUIRED_SHA256=f22143b5528e081123c3c5ed437e92f648fe69748e95fa6e2bd41484e2986cc3

if not exist %BOOST_BUILT_FILE% (

    if not exist %BOOST_TAR% (
        curl -L https://sourceforge.net/projects/boost/files/boost/1.78.0/boost_1_78_0.zip/download --output %BOOST_TAR% && (
            echo Downloaded Boost
        ) || (
            echo Error downloading boost
            GOTO :cmderr
        )
    ) else (
        echo %BOOST_TAR% already downloaded
    )
    
    set "BOOST_SHA256="
    for /f %%A in ('certutil -hashfile "%BOOST_TAR%" SHA256 ^| find /i /v ":" ') do set "BOOST_SHA256=%%A"
    
    if not "!BOOST_SHA256!"=="%BOOST_REQUIRED_SHA256%" (
        echo Invalid hash of boost.  Expected "%BOOST_REQUIRED_SHA256%" and got "!BOOST_SHA256!"
        GOTO :cmderr
    )

    if not exist %BOOST_DIR% (
        tar -xzvf %BOOST_TAR% && (
            echo Un-tarred %BOOST_TAR%
        ) || (
            echo "Failed to unzip boost"
            GOTO :cmderr
        )
    ) else (
        echo "Boost was already unzipped"
    )

    cd "%BOOST_DIR%"

    echo "Running boost bootstrap.bat"
    (
        set foundErr=1
        cmd /c .\bootstrap.bat

        if errorlevel 0 if not errorlevel 1 set "foundErr="
        if defined foundErr (
            echo "Failed to run bootstrap.bat"
            GOTO :cmderr
        )
    )


    echo "Building boost release"
    .\b2.exe runtime-link=static link=static threading=multi variant=release address-model=64 architecture=x86 --prefix="%MY_PREFIX%" --build-dir=win_build_release -j8 install && (
        rmdir /s /q win_build_release
        echo Done building boost release
    ) || (
        echo "Failed to run build boost release"
        GOTO :cmderr
    )

    if defined builddebug (
        echo "Building boost debug"
        .\b2.exe runtime-link=static link=static threading=multi variant=debug address-model=64 architecture=x86 --prefix="%MY_PREFIX%" --build-dir=win_build_debug -j8 install && (
            rmdir /s /q win_build_debug
            echo Done building boost debug
        ) || (
            echo "Failed to run build boost debug"
            GOTO :cmderr
        )
    )

    echo "Built boost!"

    cd %BUILD_DIR%

    rmdir /s /q "%BOOST_DIR%" && (
        echo Removed "%BOOST_DIR%" directory
    ) || (
        echo Failed to remove "%BOOST_DIR%" directory
    )

    echo "Built boost" > %BOOST_BUILT_FILE%
) else (
    echo "Boost was already built (%BOOST_BUILT_FILE% existed)"
)


rem Build/install Zlib
set ZLIB_TAR="zlib-1.2.13.tar.gz"
set ZLIB_DIR="zlib-1.2.13"
set ZLIB_BUILT_FILE=built_%ZLIB_DIR%
set ZLIB_REQUIRED_SHA256=b3a24de97a8fdbc835b9833169501030b8977031bcb54b3b3ac13740f846ab30

if not exist %ZLIB_BUILT_FILE% (
    git clone https://github.com/madler/zlib.git %ZLIB_DIR% && (
        echo Cloned into zlib
    ) || (
        echo "Failed to clone into zlib"
        GOTO :cmderr
    )

    cd %ZLIB_DIR%

    rem checkout zlib 1.2.13
    git checkout 04f42ceca40f73e2978b50e93806c2a18c1281fc && (
        echo Checked out zlib 1.2.13
    ) || (
        echo "Failed to checkout wanted zlib commit"
        GOTO :cmderr
    )

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX="%MY_PREFIX%" -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" .. && (
        echo Configured zlib Release
    ) || (
        echo "Failed to cmake configure zlib"
        GOTO :cmderr
    )

    if defined builddebug (
        cmake --build . --config Debug --target install && (
            echo Built/installed zlib Debug
        ) || (
            echo "Failed to build zlib debug"
            GOTO :cmderr
        )
    )


    cmake --build . --config Release --target install && (
        echo Built/installed zlib Release
    ) || (
        echo "Failed to build zlib Release"
        GOTO :cmderr
    )

    rem Get rid of zlib dll's, otherwise CMake used to have cambio link
    rem against those, instead of zlibstatic.lib and zlibstaticd.lib, although
    rem I think I fixed this - but didnt test
    del "%MY_PREFIX%\bin\zlib.dll"
    del "%MY_PREFIX%\bin\zlibd.dll"
    del "%MY_PREFIX%\lib\zlib.lib"
    del "%MY_PREFIX%\lib\zlibd.lib"

    echo "Built zlib!"
    cd %BUILD_DIR%

    rmdir /s /q "%ZLIB_DIR%" && (
        echo Removed "%ZLIB_DIR%" directory
    ) || (
        echo Failed to remove "%ZLIB_DIR%" directory
    )


    echo "Built zlib" > %ZLIB_BUILT_FILE%
) else (
    echo "Zlib was already built (%ZLIB_BUILT_FILE% existed)"
)


popd
echo "Completed Succesfully"
exit /b 0
goto :EOF

:invalidarg
echo "Usage: build_cl_dependencies_msvc2022.bat <temp path> <install path>"
exit /b 1
goto :EOF

:cmderr
echo "Error building %1, code %2"
popd
exit /b 2
goto :EOF
