/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2020/09/07 22:43:38 by coremart         ###   ########.fr       */
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
void				add_fastbin(struct s_alloc_chunk* chunk_ptr)
{
	int						index;
	struct s_fastbinlist	*tmp;

	index = ((chunk_ptr->size_n_bits & CHUNK_SIZE) >> 3) - 3; // (size - 24) / 16
	tmp = malloc_struct.fastbin[index];
	malloc_struct.fastbin[index] = (struct s_fastbinlist*)chunk_ptr;
	malloc_struct.fastbin[index]->next = tmp;
}

void			add_unsorted(struct s_binlist* chunk_ptr)
{
	struct s_binlist	*unsotedlist;

	unsotedlist = (struct s_binlist*)&malloc_struct.bin[-2];
	chunk_ptr->prev = unsotedlist->prev;
	chunk_ptr->next = unsotedlist;
	unsotedlist->prev->next = chunk_ptr;
	unsotedlist->prev = chunk_ptr;
}

void				unlink_chunk(struct s_binlist *chunk_ptr)
{
	chunk_ptr->next->prev = chunk_ptr->prev;
	chunk_ptr->prev->next = chunk_ptr->next;
}

/*
**	coelesce chunk forward and backward and unlink chunk from binlist
*/
struct s_binlist	*coalesce_chunk(struct s_binlist *chunk_ptr)
{
	struct s_binlist	*next_chunk;
	size_t				new_sz;

	next_chunk = (struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE));
	new_sz = chunk_ptr->size_n_bits;
	// chunk_ptr is not the last chunk and next_chunk is not in use
	if (!(chunk_ptr->size_n_bits & IS_LAST)
	&& !(((struct s_binlist *)((char*)next_chunk + (next_chunk->size_n_bits & CHUNK_SIZE)))->size_n_bits & PREVINUSE))
	{
		unlink_chunk(next_chunk);
		new_sz += next_chunk->size_n_bits & (CHUNK_SIZE | IS_LAST);
	}
	if (!(chunk_ptr->size_n_bits & PREVINUSE))
	{
		chunk_ptr = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
		unlink_chunk(chunk_ptr);
		new_sz += chunk_ptr->size_n_bits & (CHUNK_SIZE | PREVINUSE);
	}
	chunk_ptr->size_n_bits = new_sz;
	((struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE)))->prevsize = new_sz & CHUNK_SIZE;
	return (chunk_ptr);
}

void				do_small(struct s_binlist *chunk_ptr)
{
	chunk_ptr = coalesce_chunk(chunk_ptr);
	if ((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE) + sizeof(size_t) == malloc_struct.s_topchunk)
		malloc_struct.s_topchunk = chunk_ptr + sizeof(size_t);
	else
		add_unsorted(chunk_ptr);
}

void				free(void *ptr)
{
	struct s_alloc_chunk *chunk_ptr;

	if (ptr == NULL)
		return;
	// check if size is a mult of 16???
	// check if ptr is align on 16
	// check if already free
	chunk_ptr = (struct s_alloc_chunk*)((char*)ptr - sizeof(struct s_alloc_chunk));
	if ((chunk_ptr->size_n_bits & CHUNK_SIZE) <= TINY_TRESHOLD)
		add_fastbin(chunk_ptr);
	else if (chunk_ptr->size_n_bits >= getpagesize())
		munmap((void*)chunk_ptr, chunk_ptr->size_n_bits);
	else
		do_small((struct s_binlist*)chunk_ptr);
	//check that ptr is in an arena
	// check that next->prev = chunkptr and prev->next = chunkptr before collapsing
	//change the previnuse and prevsize of next chunk
	//collapse freed bin
}
