# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: coremart <coremart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/07/21 06:28:04 by coremart          #+#    #+#              #
#    Updated: 2021/07/27 16:09:56 by coremart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

## OS ##
UNAME := $(shell uname)

## COMPILATION ##
CFLAGS = -Wall -Wextra -Werror -pedantic-errors
DFLAGS = -MT $@ -MMD -MP -MF $(DDIR)/$*.d
LIBFLAGS = -dynamiclib -fPIC

## INCLUDE ##
HDIR = include

## SOURCES ##
SDIR = src
_SRCS = chunk_op.c free.c malloc.c realloc.c calloc.c reallocf.c malloc_size.c
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
LIBFLAGS = -dynamiclib -fPIC
endif

ifeq ($(UNAME), Linux) # Linux

CFLAGS += -std=gnu99 -fPIC
NAME = libmalloc.so
LIBFLAGS = -shared -fPIC
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
