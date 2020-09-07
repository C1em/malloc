/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2020/09/07 22:37:15 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdbool.h>
#include <limits.h>

#include <stdio.h>

__thread struct s_malloc_struct malloc_struct;

bool		new_arena(int type)
{

}

void		*large_malloc(size_t size)
{
	void		*alloc;

	size += sizeof(size_t) << 1;
	alloc =  mmap(NULL, NEXT_PAGEALIGN(size), PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE, -1, 0);
	if (alloc == (void*)-1)
		return (NULL);
	((struct s_alloc_chunk*)alloc)->size_n_bits = NEXT_PAGEALIGN(size);
	return ((void*)((char*)alloc + HEADER_SIZE));
}

void		*small_malloc(const size_t size)
{
	//check in unsorted bin and if not found move all unsorted elem to their respectives bin
	//check linked list else get from top chunk
}

void		*tiny_malloc(const size_t size)
{
	unsigned int			index;
	struct s_alloc_chunk	*ret;

	index = (size >> 3) - 3;
	if (malloc_struct.fastbin[index] != NULL)
	{
		//sethead;
		//setfoot;
		// unlink;
		return ((void*)((char*)ret + sizeof(struct s_alloc_chunk)));
	}
	//check in fastbin
	//check in unsorted bin and if not found move all unsorted elem to their respectives bin
	//check linked list else get from top chunk
}

bool		malloc_init(void)
{
	int					i;
	struct s_binlist	*first_bin;

	malloc_struct.tinyarenalist =  mmap(NULL, TINY_ARENA_SZ, PROT_READ | PROT_WRITE,
									MAP_ANON | MAP_PRIVATE, -1, 0);
	if (malloc_struct.tinyarenalist == (void*)-1)
		return(false);
	malloc_struct.t_topchunk = (char*)malloc_struct.t_topchunk + sizeof(struct s_arena);
	malloc_struct.smallarenalist =  mmap(NULL, SMALL_ARENA_SZ, PROT_READ | PROT_WRITE,
									MAP_ANON | MAP_PRIVATE, -1, 0);
	if (malloc_struct.smallarenalist == (void*)-1)
		return(false);
	malloc_struct.s_topchunk = (char*)malloc_struct.s_topchunk + sizeof(struct s_arena);
	i = 0;
	while (i < NBINS)
	{
		first_bin = (struct s_binlist*)&malloc_struct.bin[(i << 1) - 2];
		first_bin->next = first_bin;
		first_bin->prev = first_bin;
		i++;
	}
	return (true);
}

// WHEN ALLOCATE FROM THE TOP CHUNK PUT A HEADER AT THE END ????
void		*malloc(size_t size)
{
	if (malloc_struct.bin[0] == NULL)
		if (malloc_init() == false)
			return (NULL);
	if (size >= ULONG_MAX - getpagesize() - HEADER_SIZE || size == 0)
		return (NULL);
	// round up to the next mult8 and not mult16 coz prevsize can hold datas
	size = NEXT_16MULT(size);
	if (size > SMALL_TRESHOLD)
		return (large_malloc(size));
	if (size > TINY_TRESHOLD)
		return (small_malloc(size));
	return (tiny_malloc(size));
}

