#!/bin/sh
# fetch-runit.sh <dir> <package/CHANGES

set -e
export LC_ALL=C

mkdir -p $1
cd $1

git init

awk '/^runit/{print $2}; /^[0-9]+\./' |
sort -V |
while read -r version
do
	rm -rf doc etc man package src
	if curl -L http://smarden.org/runit/runit-$version.tar.gz | tar -xz
	then
		mv admin/runit-$version/* .
		rmdir admin/runit-$version
		rmdir admin
		git add .
		GIT_AUTHOR_NAME="Gerrit Pape" GIT_AUTHOR_EMAIL=pape@smarden.org \
		GIT_AUTHOR_DATE="$(sed -n 2p package/CHANGES)" \
		git commit -m "runit $version"
	fi
done
