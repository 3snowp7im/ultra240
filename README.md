# ULTRA240

ULTRA240 is a game engine designed to power retro-style platformers. It is a
minimal framework supporting a single rendering mode (240p).

Here are some things that ULTRA240 does:

* Create a game window
* Load file system resources
* Load user settings
* Detect entity collisions
* Detect player input
* Render frames
* Play audio (planned)

Here are some things that ULTRA240 does not do:

* Create the main loop
* Handle player input
* Implement movement
* Implement entity loading strategies

While I plan on eventually providing this software with an open source
license, it is currently UNLICENSED. You may use and modify it as you see fit,
however you may not distribute it in any form for any reason.

## Recommended software

* [ULTRA240 SDK](https://github.com/3snowp7im/ultra240-sdk) - Collection of
  utilities to compile [Tiled](https://mapeditor.org) project files into
  ULTRA240 binaries.
  
## Building

### Build dependencies

To install the build dependencies on Ubuntu:

```shell
$ sudo apt-get install build-essential libsdl2-dev libyaml-cpp-dev python3-glad
```

### Building a non-release version

There are currently no release versions of the code. To build from the current
working version, install the GNU autotools.

To install autotools on Ubuntu:

```shell
$ sudo apt-get install autoconf automake
```

Next, checkout the software and generate the `configure` script:

```shell
$ autoreconf --install
```

It is recommended to build the software outside the source directories:

```shell
$ mkdir build
$ cd build
$ ../configure
```

Finally, build and install the software:

```shell
$ make
$ sudo make install
```
