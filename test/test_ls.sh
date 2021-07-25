#!/bin/sh
export DYLD_INSERT_LIBRARIES=~/Projects/malloc/libmalloc.dylib
export DYLD_FORCE_FLAT_NAMESPACE=1
$@
