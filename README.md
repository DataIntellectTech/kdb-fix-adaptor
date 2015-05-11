# KDB+ FIX Engine

FIX is one of the most common protocols used within the finance industry. This repository provides a shared library
implementation of a FIX engine. The accompanying document also describes how to use this library to communicate with
the FIXimulator tool for testing purposes. The implementation is currently in an alpha state and has not been used in
a production system to date. Bugs and features requests should be made through GitHub.

The PDF documentation for this resource can be found [here][gitpdfdoc] and also on the [AquaQ Analytics][aquaqresources]
website.

Installation & Setup
--------------------

### Extra Resources
You will need to download the `q.lib` file from the code.kx.com/svn repository for Windows platforms
and place it in the lib/w32/ directory. For Linux this step is not required.

This project uses CMake 2.6+ to build across multiple platforms. It has been tested on Linux and
Windows. Execute the following commands on all platforms to create platform appropriate build
files within the `build` directory.

You will need to download and compile the QuickFIX library from [here][quickfixrepo]. You should take
the resulting binary file that is produced and place it into the `lib` directory that should be created
alongside the `include` & `src` directories.

```sh
mkdir build; cd build; cmake ..
```

### <img src="docs/icons/linux.png" height="16px"> Building on Linux

On Linux, you just need to run make install to complete the build process
and find the binary output in the `../bin` directory.

```sh
make install && cd ../bin
```

### <img src="docs/icons/windows.png" height="16px"> Building on Windows

On Windows platforms you will need to have the msbuild.exe available on your path. CMake creates
two Visual Studio projects that need to be built. The `INSTALL` project will not modify any of the
code and will just move the binaries from the `../build` directory to the `../bin` directory. An
extra `fixengine.lib` file will be produced on Windows, which can be ignored after the build process.

```bat
msbuild ./ALL_BUILD.vcxproj /p:Configuration=Release
msbuild ./INSTALL.vcxproj /p:Configuration=Release
cd ../bin
```

Running the Examples
--------------------

The resulting directory after running a build should look like this:

    bin/                    -- contains the libqtoc.[dll/so] library and makeprint.q
    build/                  -- contains the makefile/visual studio projects
    include/                -- contains the header files that are used by the libraries
    lib/                    -- contains the quickfix library & optionally the google test libraries
    src/                    -- contains the source code
    test/                   -- contains the unit tests for the source code
    CMakeLists.txt          -- the build instructions for the entire project
    README.md               -- this file

Once the build is complete, navigate to the `bin` directory and execute:

    ./run

This is a script that will load the C shared object and bind the functions to the .fix namespace for you. It
will also automatically start a local FIX acceptor & initiator session and provides an example send new single
order function to test with. It is also possible to load the shared library into your own session using the
dynamic load (:2) function:

```apl
q) .fix:(`:./lib/fixengine 2:(`load_library;1))`
q) .fix.listen[`SocketAcceptPort`SenderCompID`TargetCompID!(7070;`SellSideID;`BuySideID)]
q) .fix.connect[`SocketConnectPort`SenderCompID`TargetCompID!(7070;`BuySideID;`SellSideID)]
```

You should read the accompanying documentation for the full API and details of how to use the FIXimulator tool
for testing purposes. FIXimulator can be downloaded from [here][fiximulatorlink] and the source code can be found
[here][fiximulatorlink].

Acknowledgements
----------------

This product includes software developed by quickfixengine.org ([http://www.quickfixengine.org/][quickfixlink]).

[aquaqwebsite]: http://www.aquaq.co.uk  "AquaQ Analytics Website"
[aquaqresources]: http://www.aquaq.co.uk/resources "AquaQ Analytics Website Resources"
[gitpdfdoc]: https://github.com/AquaQAnalytics/kdb-fix-adaptor/blob/master/docs/FixSharedLibrary.pdf
[quickfixrepo]: https://github.com/quickfix/quickfix/
[quickfixlink]: http://www.quickfixengine.org/ 
[fiximulatorlink]: http://fiximulator.org/
[fiximulatorcode]: https://code.google.com/p/fiximulator/
