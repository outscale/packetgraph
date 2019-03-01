lpm_SOURCES = \
	src/npf/liblpm/src/lpm.c
lpm_OBJECTS = $(lpm_SOURCES:.c=.o)
lpm_HEADERS = -I$(srcdir)/src/npf/liblpm/src
lpm_CFLAGS = $(EXTRA_CFLAGS) -std=gnu11 -O2 -W  -Wcast-qual -Wwrite-strings -Wextra -Werror -fPIC $(PG_ASAN_CFLAGS)

cdb_SOURCES = \
	src/npf/libcdb/src/cdbr.c\
	src/npf/libcdb/src/cdbw.c\
	src/npf/libcdb/src/mi_vector_hash.c
cdb_OBJECTS = $(cdb_SOURCES:.c=.o)
cdb_HEADERS = -I$(srcdir)/src/npf/libcdb/src
cdb_CFLAGS = $(EXTRA_CFLAGS) -D__RCSID\(x\)= -std=gnu11 -O2 -W -Wextra -Werror -DNDEBUG -fPIC $(PG_ASAN_CFLAGS)

nvlist_SOURCES = \
	src/npf/nvlist/src/dnvlist.c\
	src/npf/nvlist/src/msgio.c\
	src/npf/nvlist/src/nvlist.c\
	src/npf/nvlist/src/nvpair.c
nvlist_OBJECTS = $(nvlist_SOURCES:.c=.o)
nvlist_HEADERS = -I$(srcdir)/src/npf/nvlist/src
nvlist_CFLAGS = $(EXTRA_CFLAGS) -D__RCSID\(x\)= -std=gnu11 -O2 -W -Wextra -Werror -DNDEBUG -D_GNU_SOURCE -fPIC $(PG_ASAN_CFLAGS) -D__FBSDID\(x\)=

thmap_SOURCES = \
	src/npf/thmap/src/murmurhash.c\
	src/npf/thmap/src/thmap.c\
	src/npf/thmap/src/t_stress.c
thmap_OBJECTS = $(thmap_SOURCES:.c=.o)
thmap_HEADERS = -I$(srcdir)/src/npf/thmap/src
thmap_CFLAGS = $(EXTRA_CFLAGS) -D__RCSID\(x\)= -std=gnu11 -O2 -W -Wextra -Werror -DNDEBUG -fPIC $(PG_ASAN_CFLAGS)

qsbr_SOURCES = \
	src/npf/libqsbr/src/ebr.c\
	src/npf/libqsbr/src/gc.c\
	src/npf/libqsbr/src/qsbr.c
qsbr_OBJECTS = $(qsbr_SOURCES:.c=.o)
qsbr_HEADERS = -I$(srcdir)/src/npf/libqsbr
qsbr_CFLAGS = $(EXTRA_CFLAGS) -D__RCSID\(x\)= -fPIC -std=gnu11 $(PG_ASAN_CFLAGS)

sljit_SOURCES = \
	src/npf/sljit/regexJIT.c\
	src/npf/sljit/sljitLir.c
sljit_OBJECTS = $(sljit_SOURCES:.c=.o)
sljit_HEADERS = -I$(srcdir)/src/npf/sljit
sljit_CFLAGS = $(EXTRA_CFLAGS) -DSLJIT_CONFIG_AUTO=1 -DSLJIT_VERBOSE=0 -DSLJIT_DEBUG=0 -fPIC $(PG_ASAN_CFLAGS)

bpfjit_SOURCES = \
	src/npf/bpfjit/bpfjit.c
bpfjit_OBJECTS = $(bpfjit_SOURCES:.c=.o)
bpfjit_HEADERS = -I$(srcdir)/src/npf/bpfjit $(sljit_HEADERS)
bpfjit_CFLAGS = $(EXTRA_CFLAGS) -DSLJIT_CONFIG_AUTO=1 -DSLJIT_VERBOSE=0 -DSLJIT_DEBUG=0 -fPIC $(PG_ASAN_CFLAGS)
bpfjit_LIBADD = libsljit.a

npf_SOURCES = \
	src/npf/npf/src/libnpf/npf.c
npf_OBJECTS = $(npf_SOURCES:.c=.o)
npf_HEADERS = -I$(srcdir)/src/npf/npf/src/libnpf -I$(srcdir)/src/npf/npf/src/kern/stand -I$(srcdir)/src/npf/nvlist/src $(thmap_HEADERS) $(qsbr_HEADERS) $(cdb_HEADERS) $(lpm_HEADERS)
npf_CFLAGS = $(EXTRA_CFLAGS) -std=gnu11 -O2 -g -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -D__KERNEL_RCSID\(x,y\)= -D_NPF_STANDALONE -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused  -Wno-unknown-warning-option -DNDEBUG -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE -fPIC -Wno-cast-function-type -Wno-stringop-truncation $(PG_ASAN_CFLAGS)

npfkern_SOURCES = \
	src/npf/npf/src/kern/npf.c\
	src/npf/npf/src/kern/npf_conf.c\
	src/npf/npf/src/kern/npf_ctl.c\
	src/npf/npf/src/kern/npf_handler.c\
	src/npf/npf/src/kern/npf_mbuf.c\
	src/npf/npf/src/kern/npf_bpf.c\
	src/npf/npf/src/kern/npf_ruleset.c\
	src/npf/npf/src/kern/npf_rproc.c\
	src/npf/npf/src/kern/npf_tableset.c\
	src/npf/npf/src/kern/npf_if.c\
	src/npf/npf/src/kern/npf_inet.c\
	src/npf/npf/src/kern/npf_conn.c\
	src/npf/npf/src/kern/npf_conndb.c\
	src/npf/npf/src/kern/npf_connkey.c\
	src/npf/npf/src/kern/npf_state.c\
	src/npf/npf/src/kern/npf_state_tcp.c\
	src/npf/npf/src/kern/npf_nat.c\
	src/npf/npf/src/kern/npf_alg.c\
	src/npf/npf/src/kern/npf_sendpkt.c\
	src/npf/npf/src/kern/npf_worker.c\
	src/npf/npf/src/kern/stand/bpf_filter.c\
	src/npf/npf/src/kern/stand/murmurhash.c\
	src/npf/npf/src/kern/stand/misc.c
npfkern_OBJECTS = $(npfkern_SOURCES:.c=.o)
npfkern_HEADERS = $(lpm_HEADERS) $(cdb_HEADERS) $(nvlist_HEADERS) $(thmap_HEADERS) $(qsbr_HEADERS) $(npf_HEADERS) -I$(srcdir)/src/npf/npf/src/kern/stand $(bpfjit_HEADERS)

npfkern_CFLAGS = $(EXTRA_CFLAGS)  -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_NPF_STANDALONE -D__KERNEL_RCSID\(x,y\)= -std=gnu11 -O2 -g -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused -Wno-cast-function-type -Wno-stringop-truncation -Wno-unknown-warning-option -fvisibility=hidden -DNDEBUG -fPIC -D__RCSID\(x\)=
#Trick to avoid symbol conflicts
npfkern_CFLAGS += -Dnpf_ruleset_add=_npf_ruleset_add -Dnpf_ruleset_remove=_npf_ruleset_remove -Dnpf_ruleset_remkey=_npf_ruleset_remkey -Dnpf_ruleset_flush=_npf_ruleset_flush -Dnpf_rule_setcode=_npf_rule_setcode -Dnpf_rule_getid=_npf_rule_getid -Dnpf_ext_construct=_npf_ext_construct -Dnpf_rproc_create=_npf_rproc_create -Dnpf_table_create=_npf_table_create -Dnpf_table_destroy=_npf_table_destroy -Dnpf_table_insert=_npf_table_insert -Drb_tree_init=_rb_tree_init -Drb_tree_find_node=_rb_tree_find_node -Drb_tree_find_node_geq=_rb_tree_find_node_geq -Drb_tree_find_node_leq=_rb_tree_find_node_leq -Drb_tree_insert_node=_rb_tree_insert_node -Drb_tree_iterate=_rb_tree_iterate -Drb_tree_remove_node=_rb_tree_remove_node $(PG_ASAN_CFLAGS)
npfkern_LIBADD = libqsbr.la liblpm.la libcdb.la libnvlist.a libthmap.a libbpfjit.la -ljemalloc -lpthread -lpcap

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
