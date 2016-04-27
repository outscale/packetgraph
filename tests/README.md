# run tests

You need to have to some hugepage configured:
- edit /etc/sysctl.conf
- add vm.nr_hugepages = 256
- reload: sysctl -p /etc/sysctl.conf
- sudo mkdir -p /mnt/huge
- edit /etc/fstab
- add none    /mnt/huge       hugetlbfs        defaults        0       0
- sudo mount -a

And have qemu installed (>= 2.5)
Then run tests: (may take a long time):
- make check

# run benchmarks

If you want to run benchmarks:
```
$ ./configure --with-benchmarks
$ make
$ make bench
```
