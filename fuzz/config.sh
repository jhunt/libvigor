#!/bin/bash

CC=/usr/bin/afl-gcc ./configure --disable-shared
make

mkdir -p fuzz/in fuzz/out
cat >fuzz/in/test <<EOF
config value
EOF
afl-fuzz -i fuzz/in -o fuzz/out ./fuzz-config
