#!/bin/bash

dir=$(cd "$(dirname $0)" && pwd)

if [ -s $dir/doxygen_error.txt ]
then
	cat $dir/doxygen_error.txt
	rm  $dir/doxygen_error.txt
	exit 1
fi

rm  $dir/doxygen_error.txt
if [ "$?" != "0" ]; then
	echo " Missing $dir/doxygen_error.txt file"
	exit 1
fi

exit 0
