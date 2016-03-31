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
This project uses CMake 3.2+ to support building across multiple platforms and has been tested on Linux. The CMake toolchain will generate the build artefacts required for your platform automatically.

### KDB+
A recent version of kdb+ (i.e version 3.x) should be used when working with the FIX engine. The free 32-bit version of the software can be downloaded from the [Kx Systems][kxsystemslink] website.

### <img src="docs/icons/linux.png" height="16px"> Building on Linux

Install Quickfix

```sh
$ sudo apt-get install g++ automake libtool libxml2 libxml2-dev
$ git clone https://github.com/quickfix/quickfix
$ cd quickfix
$ ./bootstrap
$ ./configure
$ make
$ sudo make install
```

Install FIX Library

This project provides a simple shell script that will handle the build process for you. It will compile all the artifacts in a /tmp folder and then copy the resulting package into your source directory. You can look at this script for an example of how to run the CMake build process manually.

```sh
$ git clone https://github.com/AquaQAnalytics/kdb-fix-adaptor
$ cd kdb-fix-adaptor
$ ./package.sh
```

A successful build will place a .tar.gz file in the fix-build directory that contains the shared object, a q script to load the shared object into the .fix namespace and some example configuration files.


Starting Servers (Acceptors) and Clients (Initiators)
----------------

The .fix.create function is common to both starting Acceptors and Initiators. 

The Acceptor is the server side component of a FIX engine that provides some sort of service by binding to a port and accepting messages. To start an acceptor you need to call the .fix.create function with acceptor as the first argument. The second argument may be either an empty list or a specified configuration, e.g :sessions/sample.ini. The .fix.create function called as an acceptor will start a background thread that will receive and validate messages and finally forward them to the .fix.onrecv function if the message is well formed.

```apl
/ q fix.q
q) .fix.create[`acceptor;()]
Defaulting to sample.ini config
Creating Acceptor
```

The Initiator is the client side component of the FIX engine and is intended to be used to connect to acceptors to send messages. To start an initiator you need to call the .fix.create function. This will create a background thread that will open a socket to the target acceptor and allow you to send/receive messages.

```apl
/ q fix.q
q) .fix.create[`initiator;`:sessions/sample.ini]
Creating Initiator
```

Sending a FIX Message
--------------------

In order to send a FIX message from an initiator to an acceptor, you can use the .fix.send function. The send is executed synchronously and will either raise a signal upon error, otherwise you can assume that the message has been received successfully by the counterparty.

In order to determine who the message will be sent to, the library will read the contents the message dictionary and look for a session on the same process that matches. The BeginString, SenderCompID and TargetCompID fields must be present in every message for this reason.

```apl
/ Session 1 - Create an Acceptor
/ q fix.q
q) .fix.create[`acceptor;()]
Defaulting to sample.ini config
Creating Acceptor

/ Session 2 - Create an Initiator
/ q fix.q
q) .fix.create[`initiator;()]
Defaulting to sample.ini config
Creating Initiator

/ We can create a dictionary
/ containing tags and message values
q) message: (8 11 35 46 ...)!("FIX.4.4";175;enlist "D";enlist "8" ...)
/ Then send it as a message
q) .fix.send[message]
```


Acknowledgements
----------------

This product includes software developed by quickfixengine.org ([http://www.quickfixengine.org/][quickfixlink]).
This software is based on ([pugixml library][pugixmllink]). pugixml is Copyright (C) 2006-2015 Arseny Kapoulkine.

[aquaqwebsite]: http://www.aquaq.co.uk  "AquaQ Analytics Website"
[aquaqresources]: http://www.aquaq.co.uk/resources "AquaQ Analytics Website Resources"
[gitpdfdoc]: https://github.com/AquaQAnalytics/kdb-fix-adaptor/blob/master/docs/FixSharedLibrary.pdf
[quickfixrepo]: https://github.com/quickfix/quickfix/
[quickfixlink]: http://www.quickfixengine.org/ 
[fiximulatorlink]: http://fiximulator.org/
[fiximulatorcode]: https://code.google.com/p/fiximulator/
[pugixmllink]: http://www.pugixml.org/
[kxsystemslink]: http://kx.com/software-download.php
