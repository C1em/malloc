#!/bin/sh
LD_PRELOAD=$HOME/libmalloc.so LD_FORCE_FLAT_NAMESPACE=1 $@
