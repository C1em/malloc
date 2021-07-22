#!/bin/sh
export DYLD_INSERT_LIBRARIES=./libmalloc.dylib
export DYLD_FORCE_FLAT_NAMESPACE=1
ls
