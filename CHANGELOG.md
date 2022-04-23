# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.6.0] - 2022-03-31
### Added
  - QuickFix, SpdLog, and PugiXML libraries are now downloaded via CMake instead of depending on system-wide installs.
    You can still use the system install of QuickFIX by setting the `USE_SYSTEM_QUICKFIX` option in CMake.
  - Library will now look for dependencies in the `lib/` or `lib64/` folder relative to its location before checking
    system paths or QHOME.
  - Support for wrapping/unwrapping generic repeating groups from FIX messages based on the specification. This can be
    enabled in the build with the `USE_GENERIC_FIX_GROUPS` option in CMake.
  - More logging for debugging purposes and configuration errors such as duplicate sessions or missing .fix.onrecv
    definition in the q session.
  - New `USE_32BIT` CMake flag to produce 32 bit binaries instead of the older `BUILD_X86` flag. The new flag will
    make sure that QuickFix, SpdLog, and PugiXML are built correctly to match.
  - Example Dockerfiles for building on RHEL7/CentOS and Ubuntu systems.

### Changed
  - Library is now called libkdbfix instead of kdbfix and the build will produce versioned shared objects. The fix.q file
    has been updated to match.
  - Default specifications are copied from the QuickFix library after build instead of maintaining a separate
    copy within this repository.
  - Bumped QuickFIX version distributed with library to [1.15.1][QuickFix-v1.15.1]
  - Bumped PugiXML version distributed with library to [1.12.1][PugiXML-v1.12.1]

### Removed
  - The create_acceptor and create_initiator functions have been removed. Users should use the general create function
    with the first arg to specify the type of application (.e.g. .fix.create[`acceptor;`:example/sessions.ini]).

### Fixed
  - More robust handling of socket errors when copying data between background and main threads.
  - Fixes for some memory leaks when decoding FIX types to K objects.
  - FIX types are now decoded based on the specification that a session is using rather than a global.
  - Removed unnecessary copies of data when copying to sockets and when decoding types. This should significantly improve performance.

[0.6.0]:   https://github.com/aquaqanalytics/kdb-fix-adaptor/compare/v0.5.1...v0.6.0
[QuickFix-v1.15.1]: https://github.com/quickfix/quickfix/releases/tag/v1.15.1
[PugiXML-v1.12.1]: https://github.com/zeux/pugixml/releases/tag/v1.12.1