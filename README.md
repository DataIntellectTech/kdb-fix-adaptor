# KDB+ FIX Engine

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

FIX is one of the most common protocols used within the finance industry. This repository provides a shared library
implementation of a FIX engine based on QuickFIX. The implementation is currently in an alpha state and has not
been used in a production system to date. Bugs and features requests should be made through GitHub.

## Quick Start

You can grab the latest release package from GitHub, extract it and start using it to process fix messages

```shell
wget https://github.com/aquaqanalytics/kdb-fix-adaptor/refs/tags/0.6.0.tar.gz
tar xvf 0.6.0.tar.gz
cd 0.6.0
q fix.q

# See the usage tutorial in the docs/ folder for examples on how to use the library
q) .fix.create[`acceptor;()]
```

You should read the [usage docs](./docs/usage.md) for some examples on how to configure the library and construct/send FIX messages.

## Building from Source

If you would like to build from source in order to contribute to the repository or to adjust the behaviour of the shared library to your own needs,
you can follow the steps below.

### Requirements
 - Build tools such as autotools, make, gcc etc. (You can install these with `yum groupinstall 'Development Tools'` on RHEL/CentOS or `apt install build-essential` on Debian/Ubuntu based systems)
 - CMake 3.14+
 - Git 2.3+
 - KDB 3.4+  (you can see details on how to install on the [Kx Systems][kxsystemslink] website)
 
### CMake Options

You can configure the build using the following CMake Flags:

| Option                 | Default | Description                                                                                     |
|------------------------|---------|-------------------------------------------------------------------------------------------------|
| USE_SYSTEM_QUICKFIX    | OFF     | *Use the system provided QuickFix install instead of downloading a matched version from GitHub* |
| USE_32BIT              | OFF     | *Generate 32-bit binaries (requires a multilib install on the build system)*                    |
| USE_GENERIC_FIX_GROUPS | **ON**  | *Expand repeating groups in FIX messages automatically*                                         |

You can pass these flags when invoking CMake to override the defaults:

```shell
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_GENERIC_FIX_GROUPS=OFF -DUSE_32BIT=ON
```

### Unix Build

To build on a unix system you can follow the steps below using cmake to generate the build files. These steps are also placed into
an example script called `package.sh` that will do this for you.

1. Clone the git repository

```shell
git clone https://github.com/aquaqanalytics/kdb-fix-adaptor.git
cd kdb-fix-adaptor
git submodule update --init --recursive
```

2. Create an out-of-source build directory and move into it

```shell
mkdir build-directory && cd build-directory
```

3. Generate the build files
```shell
# Generate a release build with optimizations enabled
#
# N.B. You can also run ccmake here to get a cli gui that will show you the available build options instead
# of passing them to cmake as command line options.
cmake .. -DCMAKE_BUILD_TYPE=Release
```

4. Run the build with make

```shell
# You can just run the default make target if you are interested in just building the binaries. The build_package
# target will create a .tar.gz file that is tagged with the version number and contains the binaries, default config,
# readme and licences.
make build_package
```

### Docker Build Environments

Two Dockerfiles are provided for building the shared library on RHEL7 and Ubuntu. Once they are built and stored in your
docker images list they will start up instantly and give you a build environment.

```shell
# Build the Ubuntu Focal 20.04 image for compatibility with Ubuntu/Debian/GLIBC 2.31
docker build -t aquaq/ubuntu docker/ubuntu

# Build the Centos 7 image for compatibility with CentOS/RHEL7/GLIBC 2.17
#
# WARN: This will take a while to build as the RHEL7 repositories don't have versions of
# git/cmake that are recent enough, so we build these from source. It's only expensive
# when we build the image, not when we use it.
#
docker build -t aquaq/centos7 docker/centos7
```

Once you have built one of the images above, you can quickly mount the source code directory into the container with
the `-v <localdir>:<containerdir>` flag to perform a build. Just running the `package.sh` script should leave you with
a build artifact for the target platform, or you can run the more detailed build steps above.

```shell
# start a shell within a centos environment - this example is also forwarding ssh on
# port 2222 so that you can attach a remote IDE or debugger. See the Docker documentation
# for more details on the command flags.
docker run --rm -it -p 2222:22 -v $(pwd):/build aquaq/centos7

# Run your build process (or just call package.sh)
[aquaq-build@4f44468d8da4 build]$ mkdir example-debug-build && cd example-debug-build
[aquaq-build@4f44468d8da4 build]$ cmake .. -DCMAKE_BUILD_TYPE=Debug
[aquaq-build@4f44468d8da4 build]$ make build_package
...
[100%] Built target build_package
[aquaq-build@4f44468d8da4 build]$ ls
...
kdbfix-0.6.0-Linux-x86_64.tar.gz

# once we exit the container the build cache && artifacts will remain as the build directory was mounted directly
[aquaq-build@4f44468d8da4 build]$ exit
```

License
-------

Copyright 2020 AquaQ Analytics

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Contact

If you have any suggestions or issues relating to the library, please [open an issue on GitHub][github-new-issue].

## Acknowledgements
* This product includes software developed by quickfixengine.org ([http://www.quickfixengine.org/][quickfixlink]). 
* This software is based on ([pugixml library][pugixmllink]). pugixml is Copyright (C) 2006-2015 Arseny Kapoulkine.

[aquaqwebsite]: http://www.aquaq.co.uk  "AquaQ Analytics Website"
[aquaqresources]: http://www.aquaq.co.uk/resources "AquaQ Analytics Website Resources"
[quickfixrepo]: https://github.com/quickfix/quickfix/
[quickfixlink]: http://www.quickfixengine.org/
[pugixmllink]: http://www.pugixml.org/
[kxsystemslink]: https://kx.com/developers/download-licenses/

[github-new-issue]: https://github.com/markrooney/kdb-templates/issues/new/choose
