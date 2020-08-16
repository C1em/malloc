/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2020/08/16 04:54:40 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdbool.h>
#include <limits.h>

bool		new_arena(int type)
{
	struct s_arena	*arena;

	if (type == TINY_ARENA)
	{
		arena =  mmap(NULL, TINY_ARENA_SZ, PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE, -1, 0);
		if (arena == (void*)-1)
			return(false);
		arena->prev = malloc_struct.tinyarenalist;
		malloc_struct.tinyarenalist = arena;

	}
	else if (type == SMALL_ARENA)
	{
		arena =  mmap(NULL, SMALL_ARENA_SZ, PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE, -1, 0);
		if (arena == (void*)-1)
			return(false);
		arena->prev = malloc_struct.smallarenalist;
		malloc_struct.smallarenalist = arena;
	}
	else
		return (false);
	arena->top_chunk = (char*)arena + sizeof(struct s_arena);
	return (true);
}

void		*large_malloc(const size_t size)
{
	void		*alloc;

	alloc =  mmap(NULL, NEXT_PAGEALIGN(size), PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE, -1, 0);
	if (alloc == (void*)-1)
		return (NULL);
	((struct s_alloc_chunk*)alloc)->size_n_previnuse = NEXT_PAGEALIGN(size);
	return ((void*)((char*)alloc + HEADER_SIZE));
}

void		*small_malloc(const size_t size)
{
	if (malloc_struct.smallarenalist == NULL)
		if (new_arena(SMALL_ARENA) == false)
			return (NULL);
	//check linked list else get from top chunk
}

void		*tiny_malloc(const size_t size)
{
	if (malloc_struct.tinyarenalist == NULL)
		if (new_arena(TINY_ARENA) == false)
			return (NULL);
	//check in unsorted bin and if not found move all unsorted elem to their respectives bin
	//check linked list else get from top chunk
}
void		*malloc(size_t size)
{
	// THINK ABOUT ROUNDING UP TO THE NEXT MULT8 AND NOT MULT16 COZ PREVSIZE CAN HOLD DATA
	if (size > ULONG_MAX - HEADER_SIZE - sizeof(size_t) || size == 0)
		return (NULL);
	size = NEXT_16MULT(size + HEADER_SIZE);
	if (size > SMALL_TRESHOLD)
		return (large_malloc(size));
	if (size > TINY_TRESHOLD)
		return (small_malloc(size));
	return (tiny_malloc(size));
}
