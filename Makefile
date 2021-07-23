# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: coremart <coremart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/07/21 06:28:04 by coremart          #+#    #+#              #
#    Updated: 2021/07/23 12:26:22 by coremart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

## COMPILATION ##
NAME = libmalloc.dylib
CFLAGS = -g -Wall -Wextra -Werror -pedantic-errors -std=c99
DFLAGS = -MT $@ -MMD -MP -MF $(DDIR)/$*.d

## INCLUDE ##
HDIR = include

## SOURCES ##
SDIR = src
_SRCS = chunk_op.c free.c malloc.c realloc.c calloc.c reallocf.c
SRCS = $(patsubst %,$(SDIR)/%,$(_SRCS))

## OBJECTS ##
ODIR = obj
_OBJS = $(_SRCS:.c=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

## DEPENDENCIES ##
DDIR = dep
_DEPS = $(_OBJS:.o=.d)
DEPS = $(patsubst %,$(DDIR)/%,$(_DEPS))

## RULES ##

all: $(NAME)

$(NAME): $(OBJS)
	gcc -o $(NAME) $(OBJS) $(CFLAGS) -dynamiclib -fPIC

$(ODIR)/%.o: $(SDIR)/%.c
	gcc $(CFLAGS) $(DFLAGS) -o $@ -c $< -I $(HDIR)

-include $(DEPS)

clean:
	@rm -f $(OBJS)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PRECIOUS: $(DDIR)/%.d
.PHONY: all clean fclean re $(NAME)
