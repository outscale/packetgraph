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

function parse_args {
    echo "" > config.mk
    echo 'c_to_o_dir = $(join $(addsuffix  $(1)/, $(dir $(2))), $(notdir $(2:.c=.o)))' >> config.mk

    if [ ! -z ${CFLAGS+x} ]; then
	echo CFLAGS: $CFLAGS
	echo CFLAGS=$CFLAGS >> config.mk
    fi

    if [ ! -z ${COMMON_CFLAGS+x} ]; then
	echo COMMON_CFLAGS: $COMMON_CFLAGS
	echo COMMON_CFLAGS=$COMMON_CFLAGS >> config.mk
    fi

    while true ; do
	case "$1" in
	    -t|--toolchain)
		echo use toolchain: $2
		eval $2
		shift 2 ;;
	    -h|--help)
		usage
		exit 0;;
	    *"+="*)
		echo $1
		echo $1 >> config.mk
		shift 1 ;;
	    *"="*)
		echo $1
		export $1
		echo $1 >> config.mk
		shift 1 ;;
	    *)
		break ;;
	esac
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
