#!/bin/bash

dir=$(cd "$(dirname $0)/.." && pwd)

if [ -e $dir/README.md ]; then
    sed -e 's/\[!\[Build Status\](https:\/\/travis-ci.org\/outscale\/packetgraph.svg?branch=master)\](https:\/\/travis-ci.org\/outscale\/packetgraph) \[!\[packetgraph documentation\](doc\/packetgraph_doc.png)\](https:\/\/outscale.github.io\/packetgraph\/doc\/master)/\\htmlonly \<a href=\"https:\/\/travis-ci.org\/outscale\/packetgraph\"\> \<img src=\"https:\/\/travis-ci.org\/outscale\/packetgraph.svg?branch=master\"\> \<\/a\>\\endhtmlonly/g' $dir/README.md > $dir/doc/README2.md
    sed -i 's/#betterfly/\\#betterfly/g' $dir/doc/README2.md
    exit 0
else
    echo "missing $dir/README.md file"
    exit 1
fi
