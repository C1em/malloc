/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2021/04/24 18:14:05 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdbool.h>
#include <limits.h>

#include <stdio.h>

__thread struct s_malloc_struct malloc_struct;

void		*large_malloc(size_t size) {

	void *alloc =  mmap(
		NULL,
		NEXT_PAGEALIGN(size),
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);

	if (alloc == (void*)-1)
		return (NULL);
	((struct s_alloc_chunk*)alloc)->size_n_bits = NEXT_PAGEALIGN(size);
	return ((void*)((char*)alloc + HEADER_SIZE));
}

void		*small_malloc(const size_t size) {

	//if large, move all fastbins to tiny unsorted + coalescing
	//check in small unsorted bin and if not found move all unsorted elem to their respectives bin + coalescing
	//check in small/large bins
	//get from top chunk
	//if not enough in top chunk create new arena and add last top chunk to bins

	// if a big small
	if (size > (SMALL_TRESHOLD + TINY_TRESHOLD) >> 1)
		//move fastbins to tiny unsorted
		;
}

static inline struct s_alloc_chunk	*check_fastbin(size_t size) {

	if (size > FASTBIN_MAX)
		return (NULL);

	unsigned int index = (size >> 4) - 2; // (size - 32) / 16
	struct s_fastbinlist *ret = malloc_struct.fastbin[index];
	if (ret == NULL)
		return (NULL);
	malloc_struct.fastbin[index] = malloc_struct.fastbin[index]->next;

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_tinybin(size_t size) {

	unsigned int index = (size >> 3) - 4; // (size - 32) / 8
	struct s_binlist *ret = (struct s_binlist*)&malloc_struct.bin[index];
	if (ret->next == ret)
		return (NULL);
	ret = ret->prev;
	unlink_chunk(ret);
	((struct s_alloc_chunk*)((char*)ret + (ret->size_n_bits & CHUNK_SIZE)))->size_n_bits |= PREVINUSE;
	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*do_fastbin(const size_t size) {

	struct s_fastbinlist	*ret;
	unsigned int			i;

	i = 0;
	while (i < (FASTBIN_MAX >> 4) - 1) {

		ret = malloc_struct.fastbin[i];
		while (ret != NULL) {
			malloc_struct.fastbin[i] = ret->next;
			ret = (struct s_fastbinlist*)coalesce_tinychunk((struct s_binlist*)ret);
			if ((ret->size_n_bits & CHUNK_SIZE) == size) {

				((struct s_alloc_chunk*)((char*)ret + (ret->size_n_bits & CHUNK_SIZE)))->size_n_bits |= PREVINUSE;
				return ((struct s_alloc_chunk*)ret);
			}
			// check if is the new top chunk ????
			add_tinybin((struct s_binlist*)ret);
			ret = malloc_struct.fastbin[i];
		}
		i++;
	}
	return (NULL);
}

struct s_alloc_chunk	*get_from_tinytopchunk(size_t size) {

	// If size is too big to fit in
	if (PTR_OFFSET(malloc_struct.topchunk_tinyarena, size + sizeof(size_t)) > PTR_OFFSET(malloc_struct.tinyarenalist, TINY_ARENA_SZ))
		return (NULL);

	struct s_alloc_chunk *topchunk = (struct s_alloc_chunk*)PTR_OFFSET(malloc_struct.topchunk_tinyarena, - (long)sizeof(size_t));

	set_chunk_size(topchunk, size);
	rm_bits(topchunk, ISTOPCHUNK);

	if ((char*)+ size +  > (char*)malloc_struct.tinyarenalist + TINY_ARENA_SZ) {

		size = (size_t)((char*)malloc_struct.tinyarenalist + TINY_ARENA_SZ - (char*)topchunk) - sizeof(struct s_alloc_chunk);
		if (size > TINY_TRESHOLD)
			size = TINY_TRESHOLD;
	}

	set_chunk_size(next_chunk(topchunk), 0);
	add_bits(next_chunk(topchunk), PREVINUSE | ISTOPCHUNK);

	malloc_struct.topchunk_tinyarena = PTR_OFFSET(topchunk, size + sizeof(size_t));
	return (topchunk);
}

struct s_alloc_chunk	*new_tinyarena(size_t size) {

	struct s_arena *new_arena = mmap(
		NULL,
		TINY_ARENA_SZ,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);
	if (new_arena == (void*)-1)
		return(NULL);

	struct s_binlist *last_chunk = (struct s_binlist*)PTR_OFFSET(malloc_struct.topchunk_tinyarena, - sizeof(size_t));
	size_t chunk_size = (size_t)malloc_struct.tinyarenalist + TINY_ARENA_SZ
	- (size_t)last_chunk - sizeof(struct s_alloc_chunk);

	// If the last part of the arena has enough space left to store a chunk
	if (chunk_size >= 32) {

		((struct s_alloc_chunk*)PTR_OFFSET(last_chunk, chunk_size))->size_n_bits = ISTOPCHUNK;

		// We can safely add the PREVINUSE bit coz we know the last chunk is not free otherwise is would be the top chunk
		last_chunk->size_n_bits = chunk_size | PREVINUSE;

		add_tinybin(last_chunk);
	}

	new_arena->prev = malloc_struct.tinyarenalist;
	malloc_struct.tinyarenalist = new_arena;

	set_chunk_size(new_arena, size);
	add_bits(new_arena, PREVINUSE);

	add_bits(PTR_OFFSET(new_arena, size), PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_tinyarena = PTR_OFFSET(new_arena, size + sizeof(size_t));
	return((struct s_alloc_chunk*)new_arena);
}

void		*tiny_malloc(size_t size) {

	struct s_alloc_chunk *(*malloc_strategy[5])(size_t) = {
		check_fastbin,
		check_tinybin,
		do_fastbin,
		get_from_tinytopchunk,
		new_tinyarena
	};

	struct s_alloc_chunk *ret = NULL;
	for (unsigned int i = 0; i < sizeof(malloc_strategy) / sizeof(malloc_strategy[0]); i++) {

		ret = malloc_strategy[i](size);
		if (ret != NULL)
			return (PTR_OFFSET(ret, HEADER_SIZE));
	}
	return (NULL);
}

bool		malloc_init(void) {

	malloc_struct.tinyarenalist =  mmap(
		NULL,
		TINY_ARENA_SZ,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);

	if (malloc_struct.tinyarenalist == (void*)-1)
		return(false);
	add_bits(malloc_struct.tinyarenalist, PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_tinyarena = PTR_OFFSET(malloc_struct.tinyarenalist, sizeof(struct s_arena));

	malloc_struct.smallarenalist =  mmap(
		NULL,
		SMALL_ARENA_SZ,
		PROT_READ| PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);

	if (malloc_struct.smallarenalist == (void*)-1)
		return (false);
	add_bits(malloc_struct.smallarenalist, PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_smallarena = PTR_OFFSET(malloc_struct.smallarenalist, sizeof(struct s_arena));

	// Each bin is a looping list where bin[0] is the next elem and bin[1] is the prev elem
	// then bin[2] is the next elem and bin[3] is the prev
	struct s_binlist *init_bin;
	for (unsigned int i = 0; i < NBINS; i++) {

		init_bin = (struct s_binlist*)&malloc_struct.bin[(i << 1) - 2];
		init_bin->next = init_bin;
		init_bin->prev = init_bin;
	}
	return (true);
}

void		*malloc(size_t size) {
	if (size >= ULONG_MAX - getpagesize() - HEADER_SIZE || size == 0)
		return (NULL);
	if (malloc_struct.bin[0] == NULL)
		if (malloc_init() == false)
			return (NULL);

	size_t chunk_size =  chunk_size_from_user_size(size);

	if (chunk_size > SMALL_TRESHOLD)
		return (large_malloc(chunk_size));
	if (chunk_size > TINY_TRESHOLD)
		return (small_malloc(chunk_size));
	return (tiny_malloc(chunk_size));
}

