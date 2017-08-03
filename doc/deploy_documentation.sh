#! /bin/bash

dir=$(cd "$(dirname $0)" && pwd)

if [ "${TRAVIS_PULL_REQUEST}" != "false" -o "${TRAVIS_BRANCH}" != "master" ]; then
    echo "Just build..."
    exit 0;
fi

# Get the current GitHub pages branch
git clone https://github.com/outscale/outscale.io-packetgraph.git hosting
# Clean hosting existing contents
rm -fr hosting/packetgraph || exit 0
cd hosting
mkdir -p packetgraph/doc/master
echo '<!DOCTYPE html><html><head><meta http-equiv="refresh" content="1; URL=https://github.com/outscale" /></head><body></body></html>' > index.html
echo '<!DOCTYPE html><html><head><meta http-equiv="refresh" content="1; URL=https://github.com/outscale/packetgraph" /></head><body></body></html>' > packetgraph/index.html
echo '<!DOCTYPE html><html><head><meta http-equiv="refresh" content="1; URL=https://outscale.github.io/packetgraph/doc/master/"/></head><body></body></html>' > packetgraph/doc/index.html

if [ -d "$dir/html" ] && [ -s "$dir/html/index.html" ]; then
    cp -r $dir/html/* packetgraph/doc/master/
    echo 'Uploading documentation to the master branch...'
    # Add everything in this directory (the Doxygen code documentation)
    git add -A .
    # Commit the added files with a title and description containing the Travis CI
    git commit -m "Deploy code docs to GitHub Pages\
    \
    Travis build info:\
    branch name ${TRAVIS_BRANCH}\
    build number${TRAVIS_BUILD_NUMBER}"
    # Force push to the remote master branch.
    git push --force "https://${GH_PACKETGRAPH_TOKEN}@${GH_REPO_REF}" master:master
else
    echo 'Warning: No documentation (html) files have been found!' >&2
    echo 'Warning: Not going to push the documentation to GitHub!' >&2
    exit 1
fi
