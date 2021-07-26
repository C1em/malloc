#!/bin/sh
DYLD_INSERT_LIBRARIES=/Users/Ciem/Projects/malloc/libmalloc.dylib DYLD_FORCE_FLAT_NAMESPACE=1 $@
