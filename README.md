# Cambio
Cambio converts spectrum files from nearly all common handheld and lab-based spectroscopic gamma radiation detectors, radiation portal monitors, or search systems to a format of your choice ([N42](https://www.nist.gov/programs-projects/ansiieee-n4242-standard), [PCF](https://prod-ng.sandia.gov/techlib-noauth/access-control.cgi/2017/179107.pdf), CSV, TXT, CHN, SPC, HTML, and more).

Cambio can be built either as a GUI that allows previewing the spectrum files, editing the meta-data, and interactively choosing the output format options, or Cambio can be built as a command line only utility.  The command line utility is useful for calling from batch scripts, or calling from other programs to take care of reading the hundreds of potential formats, making it so your program only needs to read in a single format.

## Note
The GUI version of cambio is no longer actively maintained - please see [InterSpec](https://github.com/sandialabs/InterSpec) as a far more capable replacement.

The command line version will continue to be maintained.

If you want to incorporate spectrum file parsing and manipulation into your own programs or scripts, see [SpecUtils](https://github.com/sandialabs/SpecUtils) which has bindings for a number of languages, or for Python the library can be installed through `pip`; see https://pypi.org/project/SandiaSpecUtils/.

## Getting Started
To get a pre-compiled executable, head over to the [releases](https://github.com/sandialabs/cambio/releases) tab.

## Building from source
### Prerequisites
To compile, you need a c++14 compiler, and:
* [boost](https://www.boost.org/) versions 1.65 through 1.84 are known to work, but older or newer version may work to.
* To compile the GUI, you will need [Qt](https://www.qt.io/).  
[Qt](https://www.qt.io/) is not needed for the command line only version. 
Development is done using [Qt](https://www.qt.io/) 5.15.
* [cmake](https://cmake.org/)
* [SpecUtils](https://github.com/sandialabs/SpecUtils)

### Compiling
On Linux and macOS:
```bash
git clone https://github.com/sandialabs/cambio
cd cambio
git clone https://github.com/sandialabs/SpecUtils

#build GUI version
mkdir build_gui
cd build_gui
cmake -DBUILD_CAMBIO_GUI=ON -DMyQT_DIR=/path/to/qt -DBOOST_ROOT=/path/to/boost ..
make -j8

#build command line only version
cd ..
mkdir build_cl
cd build_cl
cmake -DBUILD_CAMBIO_COMMAND_LINE=ON -DBUILD_CAMBIO_GUI=OFF -DBOOST_ROOT=/path/to/boost ..
make -j8
```
And on Windows an analogous process can be used to generate Visual Studio project with the CMake GUI.  Currently [CMakeLists.txt](CMakeLists.txt) is hardcoded to link to the static runtime on Windows, and statically compiled Qt.

### Installing
No installation is necessary - simply download or build the executable and run it.

## Author
William Johnson

## License
This project is licensed under the LGPL v2.1 License - see the [LICENSE](LICENSE) file for details

## Copyright
Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
Under the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains certain rights in this software.

## Privacy Policy
Cambio does not collect any user information or statistics, and it does not  send or receive any information over the network (i.e., nothing is downloaded to, or leaves from your computer).

SCR #988
