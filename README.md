libmsr
======

libmsr is a library for interacting with magnetic stripe reader/writers.

It's a fork of Jacob Appelbaum's [libmsr](https://github.com/ioerror/libmsr),
which has been inactive for several years and contains several incomplete
and/or buggy components. This project is an attempt to deliver a more complete
and programmer-friendly library.

### Installation and Use

```bash
$ git clone https://github.com/woodruffw/libmsr && cd libmsr
$ make && sudo make install
```

Doxygen generated documentation can be found
[here](https://yossarian.net/docs/libmsr/).

For functional examples, see the
[msr-utils](https://github.com/woodruffw/msr-utils) repository.

Once installed, linking `libmsr` into your project is as simple as adding
`-lmsr` to your linker flags.

### Hardware Support

`libmsr` currently supports the MSR-206 and all firmware-compatible
reader/writers like the MSR-505(C) and maybe the MSR-605. I've only tested
it with the MSR-505C.
