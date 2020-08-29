/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2020/08/29 14:05:35 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"

void		add_fastbin(struct s_alloc_chunk* chunk_ptr)
{
	int						index;
	struct s_fastbinlist	*tmp;

	index = ((chunk_ptr->size_n_previnuse & CHUNK_SIZE) - 24) / 16;
	tmp = malloc_struct.fastbin[index];
	malloc_struct.fastbin[index] = chunk_ptr;
	malloc_struct.fastbin[index]->next = tmp;
}

void		free(void *ptr)
{
	struct s_alloc_chunk *chunk_ptr;

	if (ptr == NULL)
		return;
	// check if size is a mult of 16???
	// check if ptr is align on 16
	chunk_ptr = (struct s_alloc_chunk*)((char*)ptr - sizeof(struct s_alloc_chunk));
	if (chunk_ptr->size_n_previnuse & CHUNK_SIZE <= TINY_TRESHOLD)
		return (add_fastbin(chunk_ptr));
	if (chunk_ptr->size_n_previnuse >= getpagesize())
	{
		munmap((void*)chunk_ptr, chunk_ptr->size_n_previnuse);
		return ;
	}
	//check that ptr is in an arena
	// check that next->prev = chunkptr and prev->next = chunkptr before collapsing
	//change the previnuse and prevsize of next chunk
	//collapse freed bin
}
