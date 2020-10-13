# EyeLink Latency Characterization

A set of simple hardware and software used to characterize end-to-end latency of
an EyeLink 1000 eye tracker and the Xubuntu 18.04 graphics stack with
compositing disabled.

## Getting Started

### Prerequisites

* **EyeLink Developers Kit for Linux**
  Steps to install:

  1. Add signing key
     ```
     $ wget -O - "http://download.sr-support.com/software/dists/SRResearch/SRResearch_key" | sudo apt-key add -
     ```
  2. Add apt repository
     ```
     $ sudo add-apt-repository "deb http://download.sr-support.com/software SRResearch main"
     $ sudo apt-get update
     ```
  3. Install latest release of EyeLink Developers Kit for Linux
     ```
     $ sudo apt-get install eyelink-display-software
     ```
     Alternatively, a tar of DEBs is available [at this link][debs].

* **Network Configuration on Host PC**
  The EyeLink PC and the Host PC communicate via a direct Ethernet connection
  between the two computers. In order for third party programming
  packages/languages to use this connection, you must use a static IP address for
  the Ethernet port that is used to connect to the Host PC. This static IP should
  be:

  ```
  IP Address: 100.1.1.2
  Subnet Mask: 255.255.255.0
  ```
* **Host Software Dependencies**
  To install compiler and library dependencies on Debian/Ubuntu:

  ```
  $ sudo apt install build-essential autotools-dev autoconf automake libgl-dev libglfw3-dev libglew-dev libglu-dev
  ```
* **Arduino IDE**
  To install and set up the Arduino IDE, follow [this guide][arduino-guide], and
  pay special attention to setting serial port permissions.

### Host Software

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

This will log the timing results to a file (results.csv) that can be analysed
afterwords to visualize the latency distributions. The CSV file format is

```
e2e (us),eyelink (us),drawing (us)
6852,1820,635
9528,1616,595
7180,1656,618
5504,1194,632
...
```

Main source code to read: [src/frontend/example.cc](src/frontend/example.cc).

### Artificial Saccade Generator Software

To setup the Arduino for use as the artificial saccade generator, use Arduino
IDE on the Host computer to program the Arduino with the script found in
[scripts/arduino.ino](scripts/arduino.ino).

## Reference

The following sections are information-oriented. They detail the specifics of
the setup.

### End-to-end Latency Measurement

**Bill of Materials**

* [1x] Artificial Saccade Generator (ASG), described below.
* [1x] Computer running Xubuntu 18.04 with compositing DISABLED.
* [1x] Dell P2815Q Display running 1920x1080 @ 240Hz using Zisworks x28 R2 kit.

After the artificial saccade, we use a host computer to continuously monitor for
gaze position changes. Once a change is detected, we use OpenGL to quickly
switch a portion of the display white. A photodiode is mounted to the area of
the display that will change. Once the photodiode triggers on the display
change, we take the time difference between the display change event and
toggling the IR LEDs as a measure of the end-to-end latency of the system. The
sequence diagram below gives an overview of the process.

```
           ┌────┐                            ┌───┐                        ┌───────┐
           │Host│                            │ASG│                        │Display│
           └─┬──┘                            └─┬─┘                        └───┬───┘
             │        Switch Eye Command       │                              │
             │ ────────────────────────────────>                              │
             │                                 │                              │
             │                 ╔═══════════════╧════════════════╗             │
             │                 ║toggle IR LED, t_start = now() ░║             │
             │                 ╚═══════════════╤════════════════╝             │
             │────┐                                                           │
             │    │ poll EyeLink for new samples                              │
             │<───┘                                                           │
             │                                 │                              │
             │                                 │────┐
             │                                 │    │ sample = photodiode level
             │                                 │<───┘
             │                                 │                              │
╔════════════╧═══════════════╗                 │                              │
║new EyeLink Sample > DELTA ░║                 │                              │
╚════════════╤═══════════════╝                 │                              │
             │                         draw white box                         │
             │ ───────────────────────────────────────────────────────────────>
             │                                 │                              │
             │              ╔══════════════════╧════════════════════╗         │
             │              ║sample > trigger level, t_end = now() ░║         │
             │              ╚══════════════════╤════════════════════╝         │
             │         t_end - t_start         │                              │
             │ <────────────────────────────────                              │
           ┌─┴──┐                            ┌─┴─┐                        ┌───┴───┐
           │Host│                            │ASG│                        │Display│
           └────┘                            └───┘                        └───────┘
```

A table clamp is used to secure the ASG to the table top so that it is easy to
tune the resistance of the potentiometers, and pupil sizes (i.e., changing
EyeLink settings, or printing different sized black dots on paper), such that
the EyeLink detects the artificial pupil reliably. This is shown below. We also
found that we needed to block some of the EyeLink's IR LED array so that the
reflection off of the white paper did not hide the IR LEDs from the ASG.

![setup][setup]

### Artificial Saccade Generator

To characterize the end-to-end latency, we use an artificial saccade generator,
which works by tricking the eye tracker to detect an abrupt change in the gaze
position of an artificial eye which does not move (a technique described in
[[1]]). The key insight is that most video-based eye trackers compute gaze
position by using the position of the pupil (emulated by a black dot laser
printed on white copy paper) as well as the position of a corneal reflection
(emulated by an infrared LED). By using two IR LEDs, we can programmatically
trigger an artificial saccade by switching one IR LED off, while simultaneously
switching the other IR LED on.

We implement this using an Arduino Uno with a custom circuit soldered to a
prototyping shield.

**Bill of Materials**

* [1x] F12N10L N-mosfet
* [1x] 1RF9530 P-mosfet
* [2x] Trim potentiometers
* [1x] 2.2 M-ohm resistor
* [1x] 22pF capacitor
* [2x] Gikfun 5mm 940nm IR LEDs
* [1x] FDS100 Photodiode
* [1x] Mount + standoffs
* [1x] Arduino Uno
* [1x] Adafruit Proto Shield for Arduino
* [1x] Black pupil laser printed on white paper

The circuit that is soldered to the Arduino Proto Shield is shown below.

![schematic][logo]

The proto shield is then mounted to the Arduino, and the two are then mounted to
an acrylic piece of plastic, with two holes cut to allow the IR LEDs to shine
through from behind the artificial eye. The laser cutter SVG can be found in
[docs/mount.svg](docs/mount.svg).

## Example Results

Using an oscilloscope, you can then time the change of pixels on the display.
Using our particular setup, we saw waveforms like the one below.

![oscope][oscope]

[1]: https://www.ncbi.nlm.nih.gov/pubmed/24771998
[arduino-guide]: https://www.arduino.cc/en/guide/linux
[debs]: http://download.sr-support.com/linuxDisplaySoftwareRelease/eyelink-display-software_1.11_x64_debs.tar.gz
[logo]: docs/circuit.png
[setup]: docs/setup.jpg
[oscope]: docs/latency_oscope.png
