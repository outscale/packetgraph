#!/bin/bash

NB_TOOLCHAIN=0

function usage {
    echo "usage: ./configure [options]"
    echo "options:"
    echo "-h --help"
    echo "-t --toolchain <TOOLCHAIN>"
    for i in "${TOOLCHAINS[@]}"
    do
	echo "		$i"
    done
}

function add_toolchain {
    if [ ! $NB_TOOLCHAIN ]; then
	export TOOLCHAINS[$NB_TOOLCHAIN]="$1"
    else
	TOOLCHAINS[$NB_TOOLCHAIN]="$1"
    fi
    echo add toolchain ${TOOLCHAINS[$NB_TOOLCHAIN]}
    NB_TOOLCHAIN=$(($NB_TOOLCHAIN + 1))
}

function var_add {
    eval "[ ! -z \${$1+x} ]"
    IS_LIB_HERE=$?
    if [ $IS_LIB_HERE -eq 0 ]; then
	TOADD="\$$1"
    else
	TOADD="$2"
	eval "export $1='$2'"
    fi
    echo -n "$1 = "
    eval "echo $TOADD"
    echo -n "$1 = " >> config.mk
    eval "echo $TOADD" >> config.mk
}

function var_append {
    eval "export $1=\"\$$1 $2\""
    eval "echo change $1 to: \$$1"
    echo -n "$1 = " >> config.mk
    eval "echo \$$1" >> config.mk
}

function define_add {
    echo -n "add define: "
    var_add $1 $2
    echo -n "COMMON_CFLAGS+=-D$1=" >> config.mk
    eval "echo \$$1" >>  config.mk
}


function str_define_add {
    echo -n "add string define: "
    var_add $1 $2
    echo -n "COMMON_CFLAGS+=-D$1=" >> config.mk
    eval "echo \\\\\\\"\$$1\\\\\\\"" >>  config.mk
}
