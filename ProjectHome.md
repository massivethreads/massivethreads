MassiveThreads is a lightweight thread library designed for the tasking layers of high productivity languages. It has 3 key characteristics to achieve performance and make runtime implementation simple, good scheduling for recursive task parallelism, socket I/O multiplexing, and pthread-compatible API.

MassiveThreads is distributed under 2-clause BSD license.

Supported platforms:

It currently supports the following platforms (CPU, OS, compiler)
  * x86\_64  Linux gcc
  * x86\_64  Linux icc
  * XeonPhi Linux gcc in MPSS (cross compilation)
  * XeonPhi Linux icc in MPSS (cross compilation)
  * Sparc v9 Linux gcc

Our Sparc v9 port has been tested on Fujitsu FX10 supercomputer.

Installation:

On native compilation, installation is the usual three steps:

> ./configure;  make;  make install

For cross compilation, read docs/install.txt

Manual being written:
http://www.eidos.ic.i.u-tokyo.ac.jp/~tau/massivethreads/docs/texinfo/massivethreads.html