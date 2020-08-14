/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2020/08/14 02:55:17 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdbool.h>

bool		init_arena(void)
{
	malloc_struct.arena = mmap(NULL, ARENA_SIZE, PROT_READ | PROT_WRITE,
							MAP_ANON | MAP_PRIVATE, -1, 0);
	if (malloc_struct.arena == (void*)-1)
		return (false);
}

void		*large_malloc(const size_t size)
{
	void		*alloc;

	alloc =  mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (alloc == (void*)-1)
		return (NULL);
	((struct s_alloc_chunk*)alloc)->size_n_previnuse = size;
	return ((void*)((char*)alloc + HEADER_SIZE));
}

// check if size is too large (nearly size_t max)
void		*malloc(size_t size)
{
	if (malloc_struct.arena == NULL)
		if (init_arena() == false)
			return (NULL);
	size = NEXT_8MULT(size + HEADER_SIZE);
	if (size >= LARGE_TRESHOLD)
		return (large_malloc(size));
	//check linked list else get from top chunk
}

r--;
(x + r) & ~r
