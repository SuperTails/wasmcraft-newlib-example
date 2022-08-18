#!/bin/bash

if [ ! -d './newlib-cygwin' ]; then
	git clone --depth 1 git://sourceware.org/git/newlib-cygwin.git
fi

make
