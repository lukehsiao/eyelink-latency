OpenGL demo
===========

To install compiler and library dependencies on Debian/Ubuntu:

```
$ sudo apt install build-essential autotools-dev autoconf automake libgl-dev libglfw3-dev libglew-dev libglu-dev
```

To compile:

```
$ ./autogen.sh
$ mkdir build
$ cd build
$ ../configure
$ make -j`nproc`
```

To use (from inside `build` directory):

```
$ ./src/frontend/example
```

Main source code to read: [src/frontend/example.cc](https://github.com/keithw/gldemo/blob/master/src/frontend/example.cc)
