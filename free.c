/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2020/09/05 21:48:12 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdio.h>

extern __thread struct s_malloc_struct malloc_struct;

/*
**	add to fastbin list without changing metadata
*/
void		add_fastbin(struct s_alloc_chunk* chunk_ptr)
{
	int						index;
	struct s_fastbinlist	*tmp;

	index = ((chunk_ptr->size_n_bits & CHUNK_SIZE) >> 3) - 3; // (size - 24) / 16
	tmp = malloc_struct.fastbin[index];
	malloc_struct.fastbin[index] = (struct s_fastbinlist*)chunk_ptr;
	malloc_struct.fastbin[index]->next = tmp;
}

void		unlink_chunk(struct s_binlist *chunk_ptr)
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

void		free(void *ptr)
{
	struct s_alloc_chunk *chunk_ptr;

	if (ptr == NULL)
		return;
	// check if size is a mult of 16???
	// check if ptr is align on 16
	// check if already free
	chunk_ptr = (struct s_alloc_chunk*)((char*)ptr - sizeof(struct s_alloc_chunk));
	if ((chunk_ptr->size_n_bits & CHUNK_SIZE) <= TINY_TRESHOLD)
	{
		add_fastbin(chunk_ptr);
		return ;
	}
	if (chunk_ptr->size_n_bits >= getpagesize())
	{
		munmap((void*)chunk_ptr, chunk_ptr->size_n_bits);
		return ;
	}
	chunk_ptr = (struct s_alloc_chunk *)coalesce_chunk((struct s_binlist *)chunk_ptr);
	// if (is the new top chunk) change top chunk ptr else place in unsorted

	//check that ptr is in an arena
	// check that next->prev = chunkptr and prev->next = chunkptr before collapsing
	//change the previnuse and prevsize of next chunk
	//collapse freed bin
}
