#!/bin/bash
set -e

source error_print.sh

[ -z "$RTE_SDK" ] && error_print "RTE_SDK path is not set" && exit 1

tmp=pg_tmp

for a in ${RTE_SDK}/build/lib/librte_*.a ; do
    if [ ! "$a" == "librte_dpdk.a" ]; then
        # the last 'cut' is here to void grep to generate an error on empty output
        nm "$a" | grep -e initfn_ -e mp_hdlr_init_ | cut -f 3 -d " " | grep -v "\." | cut -f 1 >> $tmp
    fi
done

for a in ${RTE_SDK}/build/lib/librte_*.a ; do
    if [ ! "$a" == "librte_dpdk.a" ]; then
        objcopy --globalize-symbols=$tmp "$a"
    fi
done

echo -n "" > dpdk_symbols_autogen.h

while read f; do
    echo "PG_DPDK_INIT($f)" >> dpdk_symbols_autogen.h
done < "$tmp"

rm $tmp
