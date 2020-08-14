/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:04 by coremart          #+#    #+#             */
/*   Updated: 2020/08/14 02:48:15 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

# define HEADER_SIZE		16

# define TINY_TRESHOLD		64
# define SMALL_TRESHOLD		256
# define LARGE_TRESHOLD		1024

# define NEXT_8MULT(x)		(((x) + (8 & ((x) << 1 | (x) << 2 | (x) << 3)))& 0xFFFFFFFFFFFFFFF8)
# define NEXT_PW2(x)		(1 << (32 - __builtin_clz(x - 1)))

# define TINY_ARENA			NEXT_PW2((TINY_TRESHOLD + HEADER_SIZE) * 100)
# define SMALL_ARENA		NEXT_PW2((SMALL_TRESHOLD + HEADER_SIZE) * 100)

# define ARENA_SIZE			TINY_ARENA + SMALL_ARENA

# define PREVINUSE			0x1

struct s_binlist
{
	size_t				prev_size;
	size_t				size_n_previnuse;
	struct s_binlist*	next;
	struct s_binlist*	prev;
};

struct s_alloc_chunk
{
	size_t			prev_size;
	size_t			size_n_previnuse;
};

struct s_malloc_struct
{
	struct s_binlist*	bin_array[((TINY_TRESHOLD - 16) / 8) + ((SMALL_TRESHOLD) - 16)]; // ??? change array size
	void				*arena;
	void				*tiny_topchunk;
	void				*small_topchunk;
};

struct s_malloc_struct malloc_struct;

void		*malloc(size_t size);

#endif
