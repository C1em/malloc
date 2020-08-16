/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2020/08/16 04:45:32 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"

void		free_large(struct s_alloc_chunk *chunk_ptr)
{
	if ((chunk_ptr->size_n_previnuse & CHUNK_SIZE) % getpagesize() != 0)
		return ;
	munmap((void*)chunk_ptr, chunk_ptr->size_n_previnuse) == -1;
}

void		add_fastbin(struct s_alloc_chunk* chunk_ptr)
{

}

void		free(void *ptr)
{
	struct s_alloc_chunk *chunk_ptr;

	if (ptr == NULL)
		return;
	// check if size is a mult of 16???
	// check if ptr is align on 16
	chunk_ptr = (struct s_alloc_chunk*)((char*)ptr - sizeof(struct s_alloc_chunk));
	if (chunk_ptr->size_n_previnuse <= TINY_TRESHOLD)
		return (add_fastbin(chunk_ptr));
	if (chunk_ptr->size_n_previnuse >= getpagesize())
		return (free_large(chunk_ptr));
	//check that ptr is in an arena
	// check that next->prev = chunkptr and prev->next = chunkptr before collapsing
	//change the previnuse and prevsize of next chunk
	//collapse freed bin
}
