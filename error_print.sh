#!/bin/bash

error_print()
{
    cowsay -h &> /dev/null
    has_cowsay=$?

    ponysay -h &> /dev/null
    has_ponysay=$?

    if [ $has_ponysay -eq 0 ]; then
	echo -e "$1" | ponysay --pony derpysad
    elif [ $has_cowsay -eq 255 ]; then
	echo -e "$1" | cowsay -d
    else
	tput setaf 1
	echo -e "$1"
	tput sgr0
    fi
}

fail()
{
    error_print "$@"
    exit 1
}
