# ULTRA240

ULTRA240 is a minimal 2D tile game library designed for retro-style platformers.
Like game consoles of the 15kHz era, ULTRA240 is designed for rendering a 240p
view.

This project aims to be an easily integratable library and not a framework.
The reasoning is that there are plenty of existing windowing and graphics
libraries with more capabilities than I could hope to implement. Rather than
create a framework that attempts to do it all, my time is better spent
perfecting a small subset of features that relate to a specific use case.

ULTRA240 has three major features:

* Loading compiled game resources and code
* Detecting entity collisions
* Managing tileset textures in graphics hardware

## Recommended software

* [ULTRA240 SDK](https://github.com/3snowp7im/ultra240-sdk) - Collection of
  utilities to compile [Tiled](https://mapeditor.org) project files into
  ULTRA240 binaries.
  
* [Example game](https://github.com/3snowp7im/ultra240-example) - A sandbox
  level used to test new library features.
  
## Building

### Build dependencies

To install the build dependencies on Ubuntu:

```shell
$ sudo apt-get install build-essential libgl-dev
```

### Building a release version

First, unpack the release archive:

```shell
$ tar xf ultra240-x.x.x.tar.gz
$ cd ultra240-x.x.x
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

### Building a non-release version

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
