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
	((struct s_fastbinlist*)chunk_ptr)->next = malloc_struct.fastbin[index];
	malloc_struct.fastbin[index] = (struct s_fastbinlist*)chunk_ptr;
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
	if (!(next_chunk->size_n_bits & ISTOPCHUNK)
	&& !(((struct s_binlist *)((char*)next_chunk + (next_chunk->size_n_bits & CHUNK_SIZE)))->size_n_bits & PREVINUSE))
	{
		tmp = next_chunk->size_n_bits & CHUNK_SIZE;
		if (new_sz + tmp <= SMALL_TRESHOLD + 0x7)
		{
			new_sz += tmp;
			unlink_chunk(next_chunk);
		}
	}
	if (!(chunk_ptr->size_n_bits & PREVINUSE))
	{
		prev_chunk = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
		tmp = prev_chunk->size_n_bits & (CHUNK_SIZE | PREVINUSE);
		if (new_sz + tmp <= SMALL_TRESHOLD + 0x7)
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

struct s_binlist	*coalesce_tinychunk(struct s_binlist *chunk_ptr)
{
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
		if (new_sz + tmp <= TINY_TRESHOLD + 0x7)
		{
			new_sz += tmp;
			printf("colesce next\n");
			printf("addr chunk: %p, addr arena: %p, addr arena: %p\n", next_chunk, malloc_struct.tinyarenalist, malloc_struct.tinyarenalist->prev);
			unlink_chunk(next_chunk);
		}
	}
	if (!(chunk_ptr->size_n_bits & PREVINUSE))
	{
		prev_chunk = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
		tmp = prev_chunk->size_n_bits & (CHUNK_SIZE | PREVINUSE);
		if (new_sz + tmp <= TINY_TRESHOLD + 0x7)
		{
			new_sz += tmp;
			chunk_ptr = prev_chunk;
			printf("colesce prev\n");
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
	if ((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE)  + sizeof(size_t)== malloc_struct.topchunk_smallarena)
	{
		// print("new top chunk");
		while ((chunk_ptr->size_n_bits & PREVINUSE) == false)
		{
			chunk_ptr = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
			unlink_chunk(chunk_ptr);
		}
		chunk_ptr->size_n_bits |= ISTOPCHUNK;
		malloc_struct.topchunk_smallarena = (char*)chunk_ptr + sizeof(size_t);

	}
	else
		add_unsorted(chunk_ptr);
}

void				do_tiny(struct s_binlist *chunk_ptr)
{
	chunk_ptr = coalesce_tinychunk(chunk_ptr);
	if ((char*)chunk_ptr + (chunk_ptr->size_n_bits & CHUNK_SIZE)  + sizeof(size_t) == malloc_struct.topchunk_tinyarena)
	{
		// never enter ??? at the last free the progemme should enter in this
		while (!(chunk_ptr->size_n_bits & PREVINUSE))
		{
			chunk_ptr = (struct s_binlist*)((char*)chunk_ptr - chunk_ptr->prevsize);
			unlink_chunk(chunk_ptr);
		}
		// printf("change top chunk\n");
		chunk_ptr->size_n_bits |= ISTOPCHUNK;
		malloc_struct.topchunk_tinyarena = (char*)chunk_ptr + sizeof(size_t);
		printf("new top chunk : %p, arena: %p\n", malloc_struct.topchunk_tinyarena, malloc_struct.tinyarenalist);
	}
	else if (printf("add to unsorted\n"))
		add_unsorted(chunk_ptr);
}

void				free(void *ptr)
{
	struct s_alloc_chunk *chunk_ptr;

	if (ptr == NULL || malloc_struct.bin[0] == NULL)
		return;
	// check if size is a mult of 16???
	// check if ptr is align on 16
	// check if already free (previnuse of next)
	chunk_ptr = (struct s_alloc_chunk*)((char*)ptr - HEADER_SIZE);
	if ((chunk_ptr->size_n_bits & CHUNK_SIZE) <= FASTBIN_MAX && printf("add fastbin of size : %zu\n", (chunk_ptr->size_n_bits & CHUNK_SIZE)))
		add_fastbin(chunk_ptr);
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
	// change mmap_threshold
}
