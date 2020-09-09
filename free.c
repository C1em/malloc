/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2020/09/10 00:48:26 by coremart         ###   ########.fr       */
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

	index = ((chunk_ptr->size_n_bits & CHUNK_SIZE) >> 4) - 2; // (size - 32) / 16
	tmp = malloc_struct.fastbin[index];
	malloc_struct.fastbin[index] = (struct s_fastbinlist*)chunk_ptr;
	malloc_struct.fastbin[index]->next = tmp;
}

void			add_tinybin(struct s_binlist* chunk_ptr)
{
	struct s_binlist	*tinybin;
	unsigned int		index;

	//	((size - 32) / 16 + 1) * 2 - 2
	printf("size tinybin: %zu\n", chunk_ptr->size_n_bits & CHUNK_SIZE);
	index = ((chunk_ptr->size_n_bits & CHUNK_SIZE) >> 3) - 4;
	tinybin = (struct s_binlist*)&malloc_struct.bin[index];
	chunk_ptr->prev = tinybin->prev;
	chunk_ptr->next = tinybin;
	tinybin->prev->next = chunk_ptr;
	tinybin->prev = chunk_ptr;
	((struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE)))->size_n_bits &= ~PREVINUSE;
}

void			add_unsorted(struct s_binlist* chunk_ptr)
{
	struct s_binlist	*unsotedlist;

	unsotedlist = (struct s_binlist*)&malloc_struct.bin[-2];
	chunk_ptr->prev = unsotedlist->prev;
	chunk_ptr->next = unsotedlist;
	unsotedlist->prev->next = chunk_ptr;
	unsotedlist->prev = chunk_ptr;
	((struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE)))->size_n_bits &= ~PREVINUSE;
}

void				unlink_chunk(struct s_binlist *chunk_ptr)
{
	printf("get chunk in list\n");
	chunk_ptr->next->prev = chunk_ptr->prev;
	chunk_ptr->prev->next = chunk_ptr->next;
}

/*
**	coelesce chunk forward and backward and unlink chunk from binlist
*/
struct s_binlist	*coalesce_smallchunk(struct s_binlist *chunk_ptr)
{
	struct s_binlist	*next_chunk;
	struct s_binlist	*prev_chunk;
	size_t				new_sz;
	size_t				tmp;

	next_chunk = (struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE));
	new_sz = chunk_ptr->size_n_bits;
	// chunk_ptr is not the last chunk and next_chunk is not in use
	if ((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE) + sizeof(size_t) != malloc_struct.s_topchunk
	&& !(((struct s_binlist *)((char*)next_chunk + (next_chunk->size_n_bits & CHUNK_SIZE)))->size_n_bits & PREVINUSE))
	{
		tmp = next_chunk->size_n_bits & (CHUNK_SIZE);
		if (new_sz + tmp <= SMALL_TRESHOLD + 7)
		{
			new_sz += tmp;
			unlink_chunk(next_chunk);
		}
	}
	if (!(chunk_ptr->size_n_bits & PREVINUSE))
	{
		prev_chunk = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
		tmp = prev_chunk->size_n_bits & (CHUNK_SIZE | PREVINUSE);
		if (new_sz + tmp <= SMALL_TRESHOLD + 7)
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
bool				islast(struct s_binlist* chunk_ptr)
{
	(char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE) + sizeof(size_t) != malloc_struct.t_topchunk;
	//check if is not topchunk
	//if is not at the end at page return false else find arena and check if is last of arena
}

struct s_binlist	*coalesce_tinychunk(struct s_binlist *chunk_ptr)
{
	struct s_binlist	*next_chunk;
	struct s_binlist	*prev_chunk;
	size_t				new_sz;
	size_t				tmp;

	next_chunk = (struct s_binlist*)((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE));
	new_sz = chunk_ptr->size_n_bits;
	// chunk_ptr is not the last chunk and next_chunk is not in use
	// if chunk_ptr is the last from prev arena ???
	if (!islast(chunk_ptr)
	&& !(((struct s_binlist *)((char*)next_chunk + (next_chunk->size_n_bits & CHUNK_SIZE)))->size_n_bits & PREVINUSE))
	{
		tmp = next_chunk->size_n_bits & (CHUNK_SIZE);
		if (new_sz + tmp <= TINY_TRESHOLD + 7)
		{
			new_sz += tmp;
			unlink_chunk(next_chunk);
		}
	}
	if (!(chunk_ptr->size_n_bits & PREVINUSE))
	{
		prev_chunk = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
		tmp = prev_chunk->size_n_bits & (CHUNK_SIZE | PREVINUSE);
		if (new_sz + tmp <= TINY_TRESHOLD + 7)
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

void				do_small(struct s_binlist *chunk_ptr)
{
	chunk_ptr = coalesce_smallchunk(chunk_ptr);
	if ((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE) + sizeof(size_t) == malloc_struct.s_topchunk)
		malloc_struct.s_topchunk = chunk_ptr + sizeof(size_t);
	else
		add_unsorted(chunk_ptr);
}

void				do_tiny(struct s_binlist *chunk_ptr)
{
	chunk_ptr = coalesce_tinychunk(chunk_ptr);
	if ((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE)  + sizeof(size_t)== malloc_struct.t_topchunk)
		malloc_struct.t_topchunk = chunk_ptr + sizeof(size_t);
	else
		add_tinybin(chunk_ptr);
}

void				free(void *ptr)
{
	struct s_alloc_chunk *chunk_ptr;

	if (ptr == NULL)
		return;
	// check if size is a mult of 16???
	// check if ptr is align on 16
	// check if already free
	chunk_ptr = (struct s_alloc_chunk*)((char*)ptr - HEADER_SIZE);
	if ((chunk_ptr->size_n_bits & CHUNK_SIZE) <= FASTBIN_MAX)
		add_fastbin(chunk_ptr);
	// check tiny_treshold
	else if (chunk_ptr->size_n_bits >= getpagesize())
		munmap((void*)chunk_ptr, chunk_ptr->size_n_bits);
	else if ((chunk_ptr->size_n_bits & CHUNK_SIZE) <= TINY_TRESHOLD)
		do_tiny((struct s_binlist*)chunk_ptr);
	else
		do_small((struct s_binlist*)chunk_ptr);
	//check that ptr is in an arena
	// check that next->prev = chunkptr and prev->next = chunkptr before collapsing
	//change the previnuse and prevsize of next chunk
	//collapse freed bin
}
