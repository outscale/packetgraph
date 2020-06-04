# Packetgraph contribution guide

We will try to list here all the things you need to know in order to start
devlopping on packetgraph and creating new bricks.

First of all, you need to be able to build packetgraph like any user, check
[packetgraph's main page](https://github.com/outscale/packetgraph).

Here a little tree view of the project:
```
.
|-- doc
|   `-- contrib      : you are here (X)
|-- examples
|   |-- dperf
|   |-- firewall
|   |-- rxtx
|   `-- switch
|-- include
|   `-- packetgraph  : public packetgraph API
|-- src              : all internal source code built in packetgraph lib
|   |-- npf          : imported code from NPF project
|   `-- utils
`-- tests
    |-- antispoof
    |-- core
    |-- diode
    |-- firewall
    |-- integration
    |-- nic
    |-- pmtud        : Typical brick test folder, containing benchmarks
    |   |-- bench.c
    |   |-- bench.sh
    |   |-- tests.c
    |   `-- test.sh
    |-- print
    |-- queue
    |-- rxtx
    |-- style
    |-- switch
    |-- tap
    |-- vhost
    `-- vtep
```

# Before opening a pull request

In a pull request, all commits must pass all sections described below

## Build everything

As a packetgraph developper, you need to run your configure command with some
extra parameters to be sure that your modifications will make example and
benchmark build:
```
./configure --with-examples --with-benchmarks
make
```

## Run style check

Be sure that you code pass some style check by running:
```
make style
```

Some style test will run if you have some tools like `rats`, `cppcheck`,
`flawfinder` or `lizard`. The only blocking test is `checkpatch.pl` which
is embedded with the project.

See [Linux coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html).

## Run tests

To build and run all possible tests:
```
make check
```

You may not run all tests or rebuild everything when developping.

To run a specific test, for example: pmtud brick tests, you just have to call
the tests/core/test.sh from your build directory.
```
./tests/core/test.sh
```

If you just want to rebuild pmtud tests, run:
```
make tests-pmtud
```

Notes about tests:
- Some tests will need some extra commands to be available on your system like
`wget`, `qemu-system-x86_64` or `awk`.
- vhost and integration tests will spawn virtual machine.
- You will need to have [some hugepages on your system](https://github.com/outscale/butterfly#configure-huge-pages).

## Run benchmarks

Like tests, benchmarks must work. You can run all benchmarks by running:
```
make bench
```
And to run a specific benchmark (still from your build directory):
```
./tests/pmtud/bench.sh
```

# Developping a new brick

## First steps

Maybe you will have some question to ask yourself before developping a new
brick:
- Is the brick I would like to create only do the job related to it's job ?
Try not to add too much features.
- What is my brick interface ? That the first thing you may write: add a new
file in e.g. `include/packetgraph/newbrick.h` and write your interface by
looking at others to see how it's done. Comments your functions so you are
really clear on which interface you would like to propose. Don't import
external libs from public API (all files in `include/packetgraph/*`).
- Internally, bricks can be MONOPOLE which mean the brick can only have
one neighbor. That the case with nic bricks or vhost bricks. Bricks can
be DIPOLE: they can be connected only once per side. Finally, we have
MULTIPOLE bricks which can have any number of edge per side. Some bricks
like vtep can have any number of edge on one side and only one edge on the
other. So when developping a new brick trink about it's model and how it will be
used. For example, a filtering brick like a firewall will probably be a DIPOLE.
- Before coding more, you may discuss with packetgraph developpers and show them
your interface. This could avoid you an heavy refactoring later. Feel free to
contact us on irc (freenode.net #betterfly) or using github issues.
- We would like to limit external dependencies, the ideal brick is the one who
run by itself or use [dpdk's API](http://dpdk.org/doc/api/).

# Checklist

When developping a new brick, you have to check that some stuff are done before
pushing your pull request:

- Library, examples, tests and benchmarks must build
- tests and benchmarks must run successfully
- Style check must be OK, we use [linux coding style](https://www.kernel.org/doc/Documentation/CodingStyle).
- Have a clean API documentation explaining what's the brick is doing
- Add your brick in `/README.md` file
- You should provide the most important tests by creating a new folder in
`tests` folder.
- Add benchmarks with your tests (if possible)
- Add a test checking that brick cannot send an empty burst
- If your test can be run using travis (test that does not need to start qemu),
then add it to `/.travis.yml`
- Bonus: add a very simple example of your brick in `/examples` folder with
very limited external dependencies
- Don't forget to add a license to your files
- Add your headers, source files and other stuff in `/Makefile.am`
- Split your pull request in several commits so it's easy to review. Each commit
must pass tests, styles, ...

