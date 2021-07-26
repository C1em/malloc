/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2021/07/26 13:08:22 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <malloc/malloc.h>
#include <stdlib.h>

struct s_malloc_struct malloc_struct;

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

struct s_alloc_chunk	*new_smallarena(size_t size) {


	struct s_arena *new_arena = mmap(
		NULL,
		SMALL_ARENA_SZ,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);

	if (new_arena == (void*)-1)
		return(NULL);

	// end_arena - topchunk
	size_t chunk_size = (size_t)malloc_struct.smallarenalist + SMALL_ARENA_SZ
	- (size_t)malloc_struct.topchunk_smallarena;


	// We can safely set the PREVINUSE bit coz we know the last chunk is not free otherwise is would be the topchunk
	malloc_struct.topchunk_smallarena->size_n_bits = PREVINUSE;

	// If the last part of the arena has enough space left to store a chunk + a footer
	if (chunk_size >= TINY_THRESHOLD + HEADER_SIZE) {

		set_freed_chunk_size(malloc_struct.topchunk_smallarena, chunk_size - HEADER_SIZE);

		add_smallbin((struct s_binlist*)malloc_struct.topchunk_smallarena);

		malloc_struct.topchunk_smallarena = get_next_chunk(malloc_struct.topchunk_smallarena);
		malloc_struct.topchunk_smallarena->size_n_bits = 0;
		chunk_size = HEADER_SIZE;
	}

	malloc_struct.topchunk_smallarena->size_n_bits |= ISTOPCHUNK;
	set_alloc_chunk_size(malloc_struct.topchunk_smallarena, chunk_size);

	new_arena->prev = malloc_struct.smallarenalist;
	malloc_struct.smallarenalist = new_arena;

	set_alloc_chunk_size(new_arena, size);
	add_bits(new_arena, PREVINUSE);

	add_bits(get_next_chunk(new_arena), PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_smallarena = get_next_chunk(new_arena);
	return((struct s_alloc_chunk*)new_arena);
}


struct s_alloc_chunk	*get_from_smalltopchunk(size_t size) {

	struct s_alloc_chunk *topchunk = (struct s_alloc_chunk*)malloc_struct.topchunk_smallarena;

	// If size is too big to fit in
	if ((size_t)topchunk + size + HEADER_SIZE > (size_t)malloc_struct.smallarenalist + SMALL_ARENA_SZ)
		return (NULL);

	set_alloc_chunk_size(topchunk, size);
	rm_bits(topchunk, ISTOPCHUNK);

	malloc_struct.topchunk_smallarena = get_next_chunk(malloc_struct.topchunk_smallarena);
	malloc_struct.topchunk_smallarena->size_n_bits = PREVINUSE | ISTOPCHUNK;

	return (topchunk);
}


struct s_alloc_chunk	*check_big_smallbin(size_t size) {

	struct s_binlist *bin = (struct s_binlist*)&malloc_struct.smallbin[(NB_SMALLBINS * 2) - 2 - 2];
	struct s_binlist *ret = bin;

	if (ret->prev == ret) // if list empty
		return (NULL);

	// get a big enough chunk
	ret = ret->prev;
	while (ret != bin && get_chunk_size(ret) - TINY_THRESHOLD < size)
		ret = ret->prev;

	if (ret == bin) // if we didn't find a chunk
		return (NULL);

	size_t remainder_sz = get_chunk_size(ret) - size;

	set_alloc_chunk_size(ret, size);

	struct s_binlist* remainder = (struct s_binlist*)get_next_chunk(ret);
	rm_bits(remainder, BITS);
	add_bits(remainder, PREVINUSE);
	set_freed_chunk_size(remainder, remainder_sz);

	// if small enough to be in smallbin
	if (get_chunk_size(remainder) < SMALL_THRESHOLD) {

		unlink_chunk(ret);
		add_smallbin(remainder);
	}
	else {

		remainder->next = ret->next;
		remainder->prev = ret->prev;
		remainder->prev->next = remainder;
		remainder->next->prev = remainder;
	}

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_smallbin(size_t size) {

	int index = get_smallbin_index(size);
	struct s_binlist *ret = (struct s_binlist*)&malloc_struct.smallbin[index - 2];

	if (ret->next == ret) // if list empty
		return (NULL);
	if (get_chunk_size(ret->next) < size) // if the biggest too small
		return (NULL);

	if (get_chunk_size(ret->prev) >= size)
		ret = ret->prev;
	else {

		while (get_chunk_size(ret->next) > size)
			ret = ret->next;
		if (get_chunk_size(ret->next) == size)
			ret = ret->next;
	}

	unlink_chunk(ret);
	add_bits(get_next_chunk(ret), PREVINUSE);

	return ((struct s_alloc_chunk*)ret);
}

void		*small_malloc(const size_t size) {

	struct s_alloc_chunk *(*malloc_strategy[4])(size_t) = {
		check_smallbin,
		check_big_smallbin,
		get_from_smalltopchunk,
		new_smallarena
	};

	struct s_alloc_chunk *ret = NULL;
	for (unsigned int i = 0; i < sizeof(malloc_strategy) / sizeof(malloc_strategy[0]); i++) {

		ret = malloc_strategy[i](size);
		if (ret != NULL)
			return (ptr_offset(ret, HEADER_SIZE));
	}

	return (NULL);
}

static inline struct s_alloc_chunk	*check_fastbin(size_t size) {

	if (size > FASTBIN_MAX)
		return (NULL);

	int index = (int)(size >> 4) - 2; // (size - 32) / 16
	struct s_fastbinlist *ret = malloc_struct.fastbin[index];

	if (ret == NULL) // if fastbin empty
		return (NULL);

	malloc_struct.fastbin[index] = malloc_struct.fastbin[index]->next;

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_tinybin(size_t size) {

	int index = (int)((size >> 5) << 1) - 2; // ((size - 32) / 32) * 2
	struct s_binlist *ret = (struct s_binlist*)&malloc_struct.tinybin[index - 2];

	if (ret->next == ret) // if list empty
		return (NULL);

	if (get_chunk_size(ret->prev) == size)
		ret = ret->prev;
	else if (get_chunk_size(ret->next) >= size)
		ret = ret->next;
	else
		return (NULL);

	unlink_chunk(ret);
	add_bits(get_next_chunk(ret), PREVINUSE);

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_big_tinybin(size_t size) {

	struct s_binlist *bin = (struct s_binlist*)&malloc_struct.tinybin[(NB_TINYBINS * 2) - 2 - 2];
	struct s_binlist *ret = bin;

	if (ret->prev == ret) // if list empty
		return (NULL);

	ret = ret->prev;
	while (ret != bin && get_chunk_size(ret) - TINY_MIN < size)
		ret = ret->prev;

	if (ret == bin)
		return (NULL);

	size_t remainder_sz = get_chunk_size(ret) - size;

	set_alloc_chunk_size(ret, size);

	struct s_binlist* remainder = (struct s_binlist*)get_next_chunk(ret);
	rm_bits(remainder, BITS);
	add_bits(remainder, PREVINUSE);
	set_freed_chunk_size(remainder, remainder_sz);

	// if small enough to be in tinybin
	if (get_chunk_size(remainder) < TINY_THRESHOLD) {

		unlink_chunk(ret);
		add_tinybin(remainder);
	}
	else {

		remainder->next = ret->next;
		remainder->prev = ret->prev;
		remainder->prev->next = remainder;
		remainder->next->prev = remainder;
	}

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*coalesce_fastbin(size_t size) {

	struct s_fastbinlist	*ret;

	// Iter through fastbins
	for (int i = 0; i < (FASTBIN_MAX >> 4) - 1; i++) {

		ret = malloc_struct.fastbin[i];
		while (ret != NULL) {

			// Unlink ret from fastbin
			malloc_struct.fastbin[i] = ret->next;

			ret = (struct s_fastbinlist*)coalesce_tinychunk((struct s_any_chunk*)ret);

			// If the coalesced chunk is large enough to be split
			if (get_chunk_size(ret) - TINY_MIN >= size) {

				return ((struct s_alloc_chunk*)split_tinychunk_for_size(
					(struct s_any_chunk*)ret,
					size
					));
			}

			add_tinybin((struct s_binlist*)ret);
			ret = malloc_struct.fastbin[i];
		}
	}
	return (NULL);
}

struct s_alloc_chunk	*get_from_tinytopchunk(size_t size) {

	struct s_alloc_chunk *topchunk = (struct s_alloc_chunk*)malloc_struct.topchunk_tinyarena;

	// If size is too big to fit in
	if ((size_t)topchunk + size + HEADER_SIZE > (size_t)malloc_struct.tinyarenalist + TINY_ARENA_SZ)
		return (NULL);

	set_alloc_chunk_size(topchunk, size);
	rm_bits(topchunk, ISTOPCHUNK);

	malloc_struct.topchunk_tinyarena = get_next_chunk(malloc_struct.topchunk_tinyarena);
	malloc_struct.topchunk_tinyarena->size_n_bits = PREVINUSE | ISTOPCHUNK;

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

	// end_arena - topchunk
	size_t chunk_size = (size_t)malloc_struct.tinyarenalist + TINY_ARENA_SZ
	- (size_t)malloc_struct.topchunk_tinyarena;


	// We can safely set the PREVINUSE bit coz we know the last chunk is not free otherwise is would be the topchunk
	malloc_struct.topchunk_tinyarena->size_n_bits = PREVINUSE;

	// If the last part of the arena has enough space left to store a chunk + a footer
	if (chunk_size >= TINY_MIN + HEADER_SIZE) {

		set_freed_chunk_size(malloc_struct.topchunk_tinyarena, chunk_size - HEADER_SIZE);

		add_tinybin((struct s_binlist*)malloc_struct.topchunk_tinyarena);

		malloc_struct.topchunk_tinyarena = get_next_chunk(malloc_struct.topchunk_tinyarena);
		malloc_struct.topchunk_tinyarena->size_n_bits = 0;
		chunk_size = HEADER_SIZE;
	}

	malloc_struct.topchunk_tinyarena->size_n_bits |= ISTOPCHUNK;
	set_alloc_chunk_size(malloc_struct.topchunk_tinyarena, chunk_size);

	new_arena->prev = malloc_struct.tinyarenalist;
	malloc_struct.tinyarenalist = new_arena;

	set_alloc_chunk_size(new_arena, size);
	add_bits(new_arena, PREVINUSE);

	add_bits(get_next_chunk(new_arena), PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_tinyarena = get_next_chunk(new_arena);
	return((struct s_alloc_chunk*)new_arena);
}

void		*tiny_malloc(size_t size) {

	struct s_alloc_chunk *(*malloc_strategy[6])(size_t) = {
		check_fastbin,
		check_tinybin,
		check_big_tinybin,
		coalesce_fastbin,
		get_from_tinytopchunk,
		new_tinyarena
	};

	struct s_alloc_chunk *ret = NULL;
	for (unsigned int i = 0; i < sizeof(malloc_strategy) / sizeof(malloc_strategy[0]); i++) {

		ret = malloc_strategy[i](size);
		if (ret != NULL)
			return (ptr_offset(ret, HEADER_SIZE));
	}
	return (NULL);
}

bool		malloc_init(void) {

	malloc_struct.tinyarenalist =  mmap(
		NULL,
		TINY_ARENA_SZ + SMALL_ARENA_SZ,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);

	if (malloc_struct.tinyarenalist == (void*)-1)
		return(false);
	add_bits(malloc_struct.tinyarenalist, PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_tinyarena = (struct s_any_chunk*)malloc_struct.tinyarenalist;

	malloc_struct.smallarenalist = ptr_offset(malloc_struct.tinyarenalist, TINY_ARENA_SZ);

	if (malloc_struct.smallarenalist == (void*)-1)
		return (false);
	add_bits(malloc_struct.smallarenalist, PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_smallarena = (struct s_any_chunk*)malloc_struct.smallarenalist;

	// Each bin is a looping list where bin[0] is the next elem and bin[1] is the prev elem
	// then bin[2] is the next elem and bin[3] is the prev
	struct s_binlist *init_bin;
	for (int i = 0; i < NB_SMALLBINS + NB_TINYBINS; i++) {

		init_bin = (struct s_binlist*)(&(malloc_struct.smallbin[(i << 1) - 2]));
		init_bin->next = init_bin;
		init_bin->prev = init_bin;
	}
	return (true);
}

void		print_size(size_t sz) {

	char	output[21] = "00000000000000000000";
	int		i = 19;

	while (sz > 0) {

		output[i] += (sz % 10);
		sz /= 10;
		i--;
	}

	write(1, &output[i + 1], 19 - i);
}

void	print_addr(void *addr);

void		*malloc(size_t size) {

	// write(1, "malloc(", 7);
	// print_size(size);
	// write(1, "):\t", 3);

	// TODO: put a max_size
	if (size >= ULONG_MAX - PAGE_SZ - HEADER_SIZE)
		return (NULL);
	if (malloc_struct.tinybin[0] == NULL)
		if (malloc_init() == false)
			return (NULL);

	size_t chunk_size =  chunk_size_from_user_size(size);

	void	*res = NULL;
	if (chunk_size >= SMALL_THRESHOLD)
		res = large_malloc(chunk_size);
	else if (chunk_size >= TINY_THRESHOLD)
		res = small_malloc(chunk_size);
	else
		res = tiny_malloc(chunk_size);

	// print_addr(res);
	// write(1, "\n", 1);
	return (res);
}
