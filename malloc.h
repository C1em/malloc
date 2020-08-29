/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:04 by coremart          #+#    #+#             */
/*   Updated: 2020/08/29 14:05:43 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

# define HEADER_SIZE		sizeof(struct s_alloc_chunk)

# define TINY_TRESHOLD		178
# define SMALL_TRESHOLD		512

# define NEXT_8MULT(x)		((7 + x) & ~7)
# define NEXT_16MULT(x)		((15 + x) & ~15)
# define NEXT_PAGEALIGN(x)	((4095 + x) & ~4095)
# define NEXT_PW2(x)		(1 << (32 - __builtin_clz(x - 1)))

# define TINY_ARENA_SZ		NEXT_PW2((TINY_TRESHOLD + HEADER_SIZE) * 100)
# define SMALL_ARENA_SZ		NEXT_PW2((SMALL_TRESHOLD + HEADER_SIZE) * 100)

# define PREVINUSE			0x1
# define IS_IN_ARENA		0x2 //????
# define CHUNK_SIZE			~0x7UL

# define TINY_ARENA			0x0
# define SMALL_ARENA		0x1

struct s_binlist
{
	size_t				prevsize;
	size_t				size_n_previnuse;
	struct s_binlist*	next;
	struct s_binlist*	prev;
};

struct s_fastbinlist
{
	size_t					prevsize;
	size_t					size_n_previnuse;
	struct s_fastbinlist*	next;
};

struct s_alloc_chunk
{
	size_t			prevsize;
	size_t			size_n_previnuse;
};

struct s_arena
{
	void				*top_chunk;
	struct s_arena*		prev;
};

//optimize size ???
struct s_malloc_struct
{
	struct s_arena			*tinyarenalist;
	struct s_arena			*smallarenalist;
	struct s_binlist*		bin[128]; // ??? change array size one for each size of tiny arena + one for each n of small arena

	//24 40 56 72 88 104 120 146 162 178
	struct s_fastbinlist*	fastbin[10]
};

struct s_malloc_struct malloc_struct;

void		*malloc(size_t size);
void		free(void *ptr);

#endif
