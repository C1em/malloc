#!/bin/sh
LD_PRELOAD=$PWD/../libft_malloc.so LD_FORCE_FLAT_NAMESPACE=1 $@
