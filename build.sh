#!/bin/bash

code="$PWD"
opts="-O0 -g -ggdb -Wall -Werror -pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-c99-extensions"

mkdir -p gebouw

echo "Building TLACK (Terrible Language And Compiler Kit)"

cd gebouw > /dev/null
clang $opts $code/src/tlack.cpp -o tlack
cd $code > /dev/null
