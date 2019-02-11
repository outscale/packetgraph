#!/bin/bash

error_print()
{
    cowsay -h &> /dev/null
    has_cowsay=$?

    ponysay -h &> /dev/null
    has_ponysay=$?

    if [ $has_ponysay -eq 0 ]; then
		ponysay --pony derpysad $1
    elif [ $has_cowsay -eq 255 ]; then
		echo $1 | cowsay -d
    else
		tput setaf 1
		echo $1
		tput sgr0
    fi
}

