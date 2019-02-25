include npfvar.mk

lpm : $(lpm_OBJECTS)
	ar rcv liblpm.a $(lpm_OBJECTS)

$(lpm_OBJECTS) : %.o : %.c
	$(CC) -c $(lpm_CFLAGS) $(lpm_HEADERS) $< -o $@

cdb : $(cdb_OBJECTS)
	ar rcv libcdb.a $(cdb_OBJECTS)

$(cdb_OBJECTS) : %.o : %.c
	$(CC) -c $(cdb_CFLAGS) $(cdb_HEADERS) $< -o $@

nvlist : $(nvlist_OBJECTS)
	ar rcv libnvlist.a $(nvlist_OBJECTS)

$(nvlist_OBJECTS) : %.o : %.c
	$(CC) -c $(nvlist_CFLAGS) $(nvlist_HEADERS) $< -o $@

thmap : $(thmap_OBJECTS)
	ar rcv libthmap.a $(thmap_OBJECTS)

$(thmap_OBJECTS) : %.o : %.c
	$(CC) -c $(thmap_CFLAGS) $(thmap_HEADERS) $< -o $@

qsbr : $(qsbr_OBJECTS)
	ar rcv libqsbr.a $(qsbr_OBJECTS)

$(qsbr_OBJECTS) : %.o : %.c
	$(CC) -c $(qsbr_CFLAGS) $(qsbr_HEADERS) $< -o $@

sljit : $(sljit_OBJECTS)
	ar rc libsljit.a $(sljit_OBJECTS)

$(sljit_OBJECTS) : %.o : %.c
	$(CC) -c $(sljit_CFLAGS) $(sljit_HEADERS) $< -o $@

bpfjit : sljit $(bpfjit_OBJECTS)
	ar rc libbpfjit.a $(bpfjit_OBJECTS)

$(bpfjit_OBJECTS) : %.o : %.c
	$(CC) -c $(bpfjit_CFLAGS) $(bpfjit_HEADERS) $<  -o $@

npf : $(npf_OBJECTS)
	ar rcv libnpf.a $(npf_OBJECTS)

$(npf_OBJECTS) : %.o : %.c
	$(CC) -c $(npf_CFLAGS) $(npf_HEADERS) $< -o $@

npfkern : $(npfkern_OBJECTS)
	ar rcv libnpfkern.a $(npfkern_OBJECTS)

$(npfkern_OBJECTS) : %.o : %.c
	$(CC) -c $(npfkern_CFLAGS) $(npfkern_HEADERS) $< -o $@

fclean_npf: clean_npf
	rm -fv libcdb.a libnvlist.a libthmap.a liblpm.a libqsbr.a libsljit.a libbpfjit.a libnpf.a libnpfkern.a

clean_npf:
	rm -fv $(lpm_OBJECTS) $(cdb_OBJECTS) $(nvlist_OBJECTS) $(thmap_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS) 
