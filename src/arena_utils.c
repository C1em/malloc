/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arena_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 15:35:38 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 16:16:32 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <stdbool.h>
#include <sys/mman.h>

extern struct s_malloc_struct malloc_struct;

/**
 * @param arena the arena to unlink
 * @return true if succefully unlinked or false
 */
bool	unlink_largearena(struct s_arena* arena) {

	if (malloc_struct.largearenalist == arena) {

		malloc_struct.largearenalist = NULL;
		return (true);
	}

	struct s_arena* arena_iter = malloc_struct.largearenalist;
	while (arena_iter != NULL) {

		if (arena_iter->prev == arena) {

			arena_iter->prev = arena_iter->prev->prev;
			return (true);
		}

		arena_iter = arena_iter->prev;
	}

	return (false);
}

bool				is_in_arena(struct s_alloc_chunk *chunk) {

	if ((void*)chunk >= (void*)malloc_struct.tinyarenalist && (void*)chunk <= ptr_offset(malloc_struct.topchunk_tinyarena, - TINY_MIN))
		return (true);

	struct s_arena	*cur_arena = malloc_struct.tinyarenalist->prev;

	while (cur_arena != NULL) {

		if ((void*)chunk >= (void*)cur_arena
		&& (void*)chunk <= ptr_offset(cur_arena, TINY_ARENA_SZ - TINY_MIN - (long)HEADER_SIZE))
			return (true);

		cur_arena = cur_arena->prev;
	}

	if ((void*)chunk >= (void*)malloc_struct.smallarenalist && (void*)chunk <= ptr_offset(malloc_struct.topchunk_smallarena, - TINY_MIN))
		return (true);


	cur_arena = malloc_struct.smallarenalist->prev;

	while (cur_arena != NULL) {

		if ((void*)chunk >= (void*)cur_arena
		&& (void*)chunk <= ptr_offset(cur_arena, SMALL_ARENA_SZ - TINY_THRESHOLD - (long)HEADER_SIZE))
			return (true);

		cur_arena = cur_arena->prev;
	}

	return (false);
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
