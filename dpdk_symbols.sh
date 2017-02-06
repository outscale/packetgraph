#!/bin/bash
set -e

[ -z "$RTE_SDK" ] && echo "RTE_SDK path is not set" && exit 1

tmp=pg_tmp

for a in ${RTE_SDK}/build/lib/librte_*.a ; do
    if [ ! "$a" == "librte_dpdk.a" ]; then
        # the last 'cut' is here to void grep to generate an error on empty output
        nm $a | grep initfn_ | cut -f 3 -d " " | grep -v "^\." | cut -f 1 >> $tmp
    fi
done

for a in ${RTE_SDK}/build/lib/librte_*.a ; do
    if [ ! "$a" == "librte_dpdk.a" ]; then
        objcopy --globalize-symbols=$tmp $a
    fi
done

cat $tmp | while read f; do
    echo "PG_DPDK_INIT($f)" >> dpdk_symbols_autogen.h
done

rm $tmp
