This folder ease build of npf firewall and sublibraries:

- [npf](https://github.com/rmind/npf)
- [libcdb](https://github.com/rmind/libcdb)
- [libqsbr](https://github.com/rmind/libqsbr)
- [libprop](https://github.com/xtraeme/portableproplib)
- [libprop patch](http://noxt.eu/~rmind/proplib-centos-rpm.diff)
- [bpfjit](https://github.com/rmind/bpfjit.git)

# Manually build and install npf

If you don't want npf to be built with packetgraph, you can still manually install npf.
Once npf and it's dependencies installed, just run:
```
$ ./configure --without-build-npf
```

Get and install Libcdb:
```
$ git clone https://github.com/rmind/libcdb
$ cd libcd
$ make rpm
$ rpm -ihv RPMS/x86_64/libcdb-*
```

Get, patch and build and install proplib:
```
$ git clone https://github.com/xtraeme/portableproplib.git
$ cd portableproplib
$ git checkout -b patched 73e5c8346231c0c9ad508b07dc1bae7662f61d7c
$ wget http://noxt.eu/~rmind/proplib-centos-rpm.diff
$ patch -p1 < proplib-centos-rpm.diff
$ ./bootstrap
$ make
$ make install
```

Or install proplib RPM package (which contains patch) for Centos:
```
$ wget http://noxt.eu/~rmind/libprop-0.6.5-1.el7.centos.x86_64.rpm
$ rpm -ihv libprop-0.6.5-1.el7.centos.x86_64.rpm
```

Install libqsbr:
```
$ git clone https://github.com/rmind/libqsbr.git
$ cd libqsbr/pkg
$ make rpm
$ rpm -ihv RPMS/x86_64/*.rpm
```

Install bpfjit:
```
$ git clone https://github.com/rmind/bpfjit.git
$ ...TODO
```

Finally, get the standalone version of npf and build it:
```
$ git clone https://github.com/rmind/npf.git
$ cd npf/pkg
$ make clean
$ make npf
$ rpm -hvi RPMS/x86_64/npf-*
```

