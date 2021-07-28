#!/bin/sh
DYLD_INSERT_LIBRARIES=$HOME/Projects/malloc/libmalloc.dylib DYLD_FORCE_FLAT_NAMESPACE=1 $@
