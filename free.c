/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2021/07/01 03:16:14 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdio.h>
#include <stdbool.h>

extern __thread struct s_malloc_struct malloc_struct;

/*
**	add to fastbin list without changing metadata
*/
void				add_fastbin(struct s_alloc_chunk *chunk) {

	int index = (get_chunk_size(chunk) >> 4) - 2; // (size - 32) / 16
	((struct s_fastbinlist*)chunk)->next = malloc_struct.fastbin[index];
	((struct s_binlist*)chunk)->prev = NULL;
	malloc_struct.fastbin[index] = (struct s_fastbinlist*)chunk;
}

void			add_tinybin(struct s_binlist *chunk) {

	int index = (((int)get_chunk_size(chunk) >> 5) << 1) - 2; // (size - 32) / 16
	struct s_binlist *tinybin = (struct s_binlist*)&malloc_struct.tinybin[index - 2];

	chunk->prev = tinybin->prev;
	chunk->next = tinybin;
	tinybin->prev->next = chunk;
	tinybin->prev = chunk;

	rm_bits(get_next_chunk(chunk), PREVINUSE);
}


// TODO: make an unsorted for tiny and small
void			add_unsorted(struct s_binlist* chunk) {

	struct s_binlist *unsotedlist = (struct s_binlist*)&malloc_struct.tinybin[-2];
	chunk->prev = unsotedlist->prev;
	chunk->next = unsotedlist;
	unsotedlist->prev->next = chunk;
	unsotedlist->prev = chunk;
	rm_bits(get_next_chunk(chunk), PREVINUSE);
}

void				unlink_chunk(struct s_binlist *chunk) {

	chunk->next->prev = chunk->prev;
	chunk->prev->next = chunk->next;
}

/*
* coelesce chunk forward and backward and unlink chunk from binlist
*/
// TODO: Make the changes like coalesce_tinychunk()
struct s_binlist	*coalesce_smallchunk(struct s_binlist *chunk_ptr) {

	struct s_binlist	*next_chunk;
	struct s_binlist	*prev_chunk;
	size_t				new_sz;
	size_t				tmp;

	next_chunk = (struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE));
	new_sz = chunk_ptr->size_n_bits;
	// chunk_ptr is not the last chunk and next_chunk is not in use
	if (!(next_chunk->size_n_bits & ISTOPCHUNK)
	&& !(((struct s_binlist *)((char*)next_chunk + (next_chunk->size_n_bits & CHUNK_SIZE)))->size_n_bits & PREVINUSE))
	{
		tmp = next_chunk->size_n_bits & CHUNK_SIZE;
		if (new_sz + tmp <= SMALL_THRESHOLD + 0x7)
		{
			new_sz += tmp;
			unlink_chunk(next_chunk);
		}
	}
	if (!(chunk_ptr->size_n_bits & PREVINUSE))
	{
		prev_chunk = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
		tmp = prev_chunk->size_n_bits & (CHUNK_SIZE | PREVINUSE);
		if (new_sz + tmp <= SMALL_THRESHOLD + 0x7)
		{
			new_sz += tmp;
			chunk_ptr = prev_chunk;
			unlink_chunk(chunk_ptr);
		}
	}
	chunk_ptr->size_n_bits = new_sz;
	((struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE)))->prevsize = new_sz & CHUNK_SIZE;
	return (chunk_ptr);
}

/*
**	Fusion neighbours free chunks with chunk_ptr
**	chunk_ptr is returned as a free chunk (but it's not in a bin)
*/
struct s_binlist	*coalesce_tinychunk(struct s_any_chunk *chunk_ptr) {

	size_t new_sz = get_chunk_size(chunk_ptr);

	struct s_binlist *next_chunk = (struct s_binlist*)get_next_chunk(chunk_ptr);

	if (!(get_bits(next_chunk) & ISTOPCHUNK)) {

		if (!(get_bits(get_next_chunk(next_chunk)) & PREVINUSE)) {

			new_sz += get_chunk_size(next_chunk);
			unlink_chunk(next_chunk);
		}
	}
	// if there is lost space at the end of the arena
	else if (get_chunk_size(next_chunk) > HEADER_SIZE) {

		size_t lost_space = get_chunk_size(next_chunk) - HEADER_SIZE;

		new_sz += lost_space;

		struct s_any_chunk* new_top_chunk = ((struct s_any_chunk*)ptr_offset(next_chunk, lost_space));
		*new_top_chunk = (struct s_any_chunk){.size_n_bits = (HEADER_SIZE | ISTOPCHUNK)};
	}

	if (!(get_bits(chunk_ptr) & PREVINUSE)) {

		struct s_binlist *prev_chunk = (struct s_binlist*)get_prev_chunk(chunk_ptr);
		new_sz += get_chunk_size(prev_chunk);
		chunk_ptr = (struct s_any_chunk*)prev_chunk;
		unlink_chunk((struct s_binlist*)chunk_ptr);
	}

	// previnuse is always set since it's coalesced
	chunk_ptr->size_n_bits = PREVINUSE;
	set_freed_chunk_size(chunk_ptr, new_sz);

	rm_bits(get_next_chunk(chunk_ptr), PREVINUSE);

	return ((struct s_binlist*)chunk_ptr);
}

void				do_small(struct s_binlist *chunk) {

	chunk = coalesce_smallchunk(chunk);
	// split_chunk()
	// add_smallbin
}

void				do_tiny(struct s_binlist *chunk) {

	chunk = coalesce_tinychunk((struct s_any_chunk*)chunk);
	// split_chunk()
	// add_tinybin
}

void				free(void *ptr) {

	if (ptr == NULL || malloc_struct.tinybin[0] == NULL)
		return;
	// TODO: add checks:
	// check if size is a mult of 16
	// check if ptr is align on 16
	// check if already free (previnuse of next)
	struct s_alloc_chunk *chunk = (struct s_alloc_chunk*)ptr_offset(ptr, - HEADER_SIZE);
	if (get_chunk_size(chunk) <= FASTBIN_MAX)
		add_fastbin(chunk);
	else if (get_chunk_size(chunk) >= SMALL_THRESHOLD)
		munmap((void*)chunk, get_chunk_size(chunk));
	else if (get_chunk_size(chunk) < TINY_THRESHOLD)
		do_tiny((struct s_binlist*)chunk);
	else
		do_small((struct s_binlist*)chunk);
	//check that ptr is in an arena
	// check that next->prev = chunkptr and prev->next = chunkptr before collapsing
	//change the previnuse and prevsize of next chunk
	//collapse freed bin
	// change mmap_threshold
}
