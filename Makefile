# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: coremart <coremart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/07/21 06:28:04 by coremart          #+#    #+#              #
#    Updated: 2021/08/12 15:39:34 by coremart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

## OS ##
UNAME := $(shell uname)

## COMPILATION ##
CFLAGS = -Wall -Wextra -Werror -pedantic-errors -O3
DFLAGS = -MT $@ -MMD -MP -MF $(DDIR)/$*.d
LIBFLAGS = -fPIC

## INCLUDE ##
HDIR = include

## SOURCES ##
SDIR = src
_SRCS = chunk_op.c free.c malloc.c realloc.c calloc.c reallocf.c malloc_size.c show_alloc_mem.c print_utils.c bin_utils.c coalesce_chunk.c arena_utils.c
SRCS = $(patsubst %,$(SDIR)/%,$(_SRCS))

## OBJECTS ##
ODIR = obj
_OBJS = $(_SRCS:.c=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

## DEPENDENCIES ##
DDIR = dep
_DEPS = $(_OBJS:.o=.d)
DEPS = $(patsubst %,$(DDIR)/%,$(_DEPS))

ifeq ($(UNAME), Darwin) # Macos

CFLAGS += -std=c99
NAME = libmalloc.dylib
LIBFLAGS += -dynamiclib
endif

ifeq ($(UNAME), Linux) # Linux

CFLAGS += -std=gnu99 -fPIC
NAME = libmalloc.so
LIBFLAGS += -shared
endif

ifeq ($(CXX),clang)
	CFLAGS += -flto=full
else ifeq ($(CXX), gcc)
	CFLAGS += -flto -Wno-unused-result
endif


## RULES ##

all: $(NAME)

$(NAME): $(OBJS)
	gcc -o $(NAME) $(OBJS) $(CFLAGS) $(LIBFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	gcc $(CFLAGS) $(DFLAGS) -o $@ -c $< -I $(HDIR)

-include $(DEPS)

clean:
	@rm -f $(OBJS)

fclean: clean
	@rm -f $(NAME)

re: fclean all


debug: CFLAGS += -DDEBUG -g
debug: re

.PRECIOUS: $(DDIR)/%.d
.PHONY: all clean fclean re $(NAME)
