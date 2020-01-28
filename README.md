# EyeLink Latency Characterization

A set of simple hardware and software used to characterize end-to-end latency of
an EyeLink 1000 eye tracker and the Xubuntu 18.04 graphics stack.

## Getting Started

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

## Reference

### Sequence Diagram

```
             ┌────┐                      ┌───────┐                      ┌───────┐
             │Host│                      │Arduino│                      │Display│
             └─┬──┘                      └───┬───┘                      └───┬───┘
               │      Switch Eye Command     │                              │
               │ ────────────────────────────>                              │
               │                             │                              │
               │             ╔═══════════════╧════════════════╗             │
               │             ║toggle IR LED, t_start = now() ░║             │
               │             ╚═══════════════╤════════════════╝             │
               │────┐                                                       │
               │    │ poll EyeLink for samples                              │
               │<───┘                                                       │
               │                             │                              │
               │                             │────┐
               │                             │    │ sample = photodiode level
               │                             │<───┘
               │                             │                              │
  ╔════════════╧═══════════════╗             │                              │
  ║new EyeLink Sample > DELTA ░║             │                              │
  ╚════════════╤═══════════════╝             │                              │
               │                       draw white box                       │
               │ ───────────────────────────────────────────────────────────>
               │                             │                              │
               │          ╔══════════════════╧════════════════════╗         │
               │          ║sample > trigger level, t_end = now() ░║         │
               │          ╚══════════════════╤════════════════════╝         │
               │       t_end - t_start       │                              │
               │ <────────────────────────────                              │
             ┌─┴──┐                      ┌───┴───┐                      ┌───┴───┐
             │Host│                      │Arduino│                      │Display│
             └────┘                      └───────┘                      └───────┘
```
