/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:04 by coremart          #+#    #+#             */
/*   Updated: 2020/09/07 22:06:45 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>
// # include <pthread.h>

# define HEADER_SIZE		sizeof(struct s_alloc_chunk)

# define TINY_TRESHOLD		168
# define SMALL_TRESHOLD		1024

# define PAGE_SZ			getpagesize()
# define NEXT_8MULT(x)		((7 + x) & ~7)
# define NEXT_16MULT(x)		((15 + x) & ~15)
# define NEXT_PAGEALIGN(x)	((PAGE_SZ - 1 + x) & ~(PAGE_SZ - 1))
# define NEXT_PW2(x)		(1 << (32 - __builtin_clz(x - 1)))

# define TINY_ARENA_SZ		NEXT_PAGEALIGN((TINY_TRESHOLD + HEADER_SIZE) * 100)
# define SMALL_ARENA_SZ		NEXT_PAGEALIGN((SMALL_TRESHOLD + HEADER_SIZE) * 100)

# define PREVINUSE			0x1
# define IS_LAST			0x2
# define CHUNK_SIZE			~0x7UL

# define NBINS				128

struct s_binlist
{
	size_t				prevsize;
	size_t				size_n_bits;
	struct s_binlist*	next;
	struct s_binlist*	prev;
};


struct s_fastbinlist
{
	size_t					prevsize;
	size_t					size_n_bits;
	struct s_fastbinlist*	next;
};

struct s_alloc_chunk
{
	size_t			prevsize;
	size_t			size_n_bits;
};

struct s_arena
{
	struct s_arena*		prev;
};

//optimize size ???
struct s_malloc_struct
{
	unsigned int			mmap_threshold; // ??? the more you free, the more it grows
	struct s_arena			*tinyarenalist;
	void					*t_topchunk;
	struct s_arena			*smallarenalist;
	void					*s_topchunk;
	struct s_binlist*		bin[NBINS * 2]; // ??? change array size one for each size of tiny arena + one for each n of small arena

	//24 40 56 72 88 104 120 136 152 168
	struct s_fastbinlist*	fastbin[10];
};

void		*malloc(size_t size);
void		free(void *ptr);

struct s_binlist	*coalesce_chunk(struct s_binlist *chunk_ptr); // for tests ????


#endif
