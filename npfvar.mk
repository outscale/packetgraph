prop_SOURCES = \
	src/npf/proplib/src/prop_array.c\
	src/npf/proplib/src/prop_array_util.c\
	src/npf/proplib/src/prop_bool.c\
	src/npf/proplib/src/prop_data.c\
	src/npf/proplib/src/prop_dictionary.c\
	src/npf/proplib/src/prop_dictionary_util.c\
	src/npf/proplib/src/prop_ingest.c\
	src/npf/proplib/src/prop_number.c\
	src/npf/proplib/src/prop_object.c\
	src/npf/proplib/src/prop_stack.c\
	src/npf/proplib/src/prop_string.c\
	src/npf/proplib/src/prop_zlib.c\
	src/npf/proplib/src/rb.c
prop_OBJECTS = $(prop_SOURCES:.c=.o)
prop_HEADERS = -I$(srcdir)/src/npf/proplib/include $(PG_HEADERS)
prop_CFLAGS = $(EXTRA_CFLAGS) -pipe -Werror -Wall -Wextra -Wvla -Wno-overlength-strings -Wundef -Wformat=2 -Wsign-compare -Wformat-security -Wmissing-include-dirs -Wformat-nonliteral -Wold-style-definition -Wpointer-arith -Winit-self -Wdeclaration-after-statement -Wfloat-equal -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wmissing-declarations -Wmissing-noreturn -Wshadow -Wendif-labels -Wstrict-aliasing -Wwrite-strings -Wno-unused-parameter -fstack-protector-all -std=gnu11 -fPIC $(PG_ASAN_CFLAGS)

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
npf_HEADERS = -I$(srcdir)/src/npf/npf/src/libnpf -I$(srcdir)/src/npf/npf/src/kern/stand $(prop_HEADERS) $(qsbr_HEADERS) $(cdb_HEADERS) $(lpm_HEADERS)
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
	src/npf/npf/src/kern/npf_state.c\
	src/npf/npf/src/kern/npf_state_tcp.c\
	src/npf/npf/src/kern/npf_nat.c\
	src/npf/npf/src/kern/npf_alg.c\
	src/npf/npf/src/kern/npf_sendpkt.c\
	src/npf/npf/src/kern/npf_worker.c\
	src/npf/npf/src/kern/stand/bpf_filter.c\
	src/npf/npf/src/kern/stand/rb.c\
	src/npf/npf/src/kern/stand/murmurhash.c\
	src/npf/npf/src/kern/stand/misc.c\
	src/npf/npf/src/kern/stand/subr_hash.c
npfkern_OBJECTS = $(npfkern_SOURCES:.c=.o)
npfkern_HEADERS = $(lpm_HEADERS) $(cdb_HEADERS) $(prop_HEADERS) $(qsbr_HEADERS) $(npf_HEADERS) -I$(srcdir)/src/npf/npf/src/kern/stand $(bpfjit_HEADERS)

npfkern_CFLAGS = $(EXTRA_CFLAGS)  -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_NPF_STANDALONE -D__KERNEL_RCSID\(x,y\)= -std=gnu11 -O2 -g -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused -Wno-cast-function-type -Wno-stringop-truncation -Wno-unknown-warning-option -fvisibility=hidden -DNDEBUG -fPIC -D__RCSID\(x\)=
#Trick to avoid symbol conflicts
npfkern_CFLAGS += -Dnpf_ruleset_add=_npf_ruleset_add -Dnpf_ruleset_remove=_npf_ruleset_remove -Dnpf_ruleset_remkey=_npf_ruleset_remkey -Dnpf_ruleset_flush=_npf_ruleset_flush -Dnpf_rule_setcode=_npf_rule_setcode -Dnpf_rule_getid=_npf_rule_getid -Dnpf_ext_construct=_npf_ext_construct -Dnpf_rproc_create=_npf_rproc_create -Dnpf_table_create=_npf_table_create -Dnpf_table_destroy=_npf_table_destroy -Dnpf_table_insert=_npf_table_insert -Drb_tree_init=_rb_tree_init -Drb_tree_find_node=_rb_tree_find_node -Drb_tree_find_node_geq=_rb_tree_find_node_geq -Drb_tree_find_node_leq=_rb_tree_find_node_leq -Drb_tree_insert_node=_rb_tree_insert_node -Drb_tree_iterate=_rb_tree_iterate -Drb_tree_remove_node=_rb_tree_remove_node $(PG_ASAN_CFLAGS)
npfkern_LIBADD = libqsbr.la liblpm.la libcdb.la libprop.la libbpfjit.la -ljemalloc -lpthread -lpcap
