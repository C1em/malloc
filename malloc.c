/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2020/09/10 00:10:50 by coremart         ###   ########.fr       */
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

	size += sizeof(size_t);
	alloc =  mmap(NULL, NEXT_PAGEALIGN(size), PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE, -1, 0);
	if (alloc == (void*)-1)
		return (NULL);
	((struct s_alloc_chunk*)alloc)->size_n_bits = NEXT_PAGEALIGN(size);
	return ((void*)((char*)alloc + HEADER_SIZE));
}

void		*small_malloc(const size_t size)
{
	//if large, move all fastbins to tiny unsorted + coalescing
	//check in small unsorted bin and if not found move all unsorted elem to their respectives bin + coalescing
	//check in small/large bins
	//get from top chunk
	//if not enough in top chunk create new arena and add last top chunk to bins
}

struct s_alloc_chunk	*check_fastbin(unsigned int index)
{
	struct s_fastbinlist	*ret;

	ret = malloc_struct.fastbin[index];
	if (ret == NULL)
		return (NULL);
	malloc_struct.fastbin[index] = malloc_struct.fastbin[index]->next;
	return ((struct s_alloc_chunk*)ret);

}

struct s_alloc_chunk	*check_tinybin(unsigned int index)
{
	struct s_binlist	*ret;

	index <<= 1;
	ret = (struct s_binlist*)&malloc_struct.bin[index];
	if (ret->next == ret)
		return (NULL);
	ret = ret->prev;
	unlink_chunk(ret);
	((struct s_alloc_chunk*)((char*)ret + (ret->size_n_bits & CHUNK_SIZE)))->size_n_bits |= PREVINUSE;
	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*do_fastbin(const size_t size)
{
	struct s_fastbinlist	*ret;
	unsigned int			i;

	i = 0;
	while (i < (FASTBIN_MAX >> 4) - 1)
	{
		ret = malloc_struct.fastbin[i];
		while (ret != NULL)
		{
			ret = (struct s_fastbinlist*)coalesce_tinychunk((struct s_binlist*)ret);
			malloc_struct.fastbin[i] = ret->next;
			if ((ret->size_n_bits & CHUNK_SIZE) == size)
			{
				((struct s_alloc_chunk*)((char*)ret + (ret->size_n_bits & CHUNK_SIZE)))->size_n_bits |= PREVINUSE;
				return ((struct s_alloc_chunk*)ret);
			}
			add_tinybin((struct s_binlist*)ret);
			ret = malloc_struct.fastbin[i];
		}
		i++;
	}
	return (NULL);
}

struct s_alloc_chunk	*get_from_ttopchunk(size_t size)
{
	struct s_alloc_chunk	*topchunk;

	if ((char*)malloc_struct.t_topchunk + size > (char*)malloc_struct.tinyarenalist + TINY_ARENA_SZ)
		return (NULL);
	topchunk = (struct s_alloc_chunk*)((char*)malloc_struct.t_topchunk - sizeof(size_t));
	if ((char*)malloc_struct.t_topchunk + size + 32 > (char*)malloc_struct.tinyarenalist + TINY_ARENA_SZ)
	{
		size = ((size_t)((char*)malloc_struct.tinyarenalist + TINY_ARENA_SZ - (char*)topchunk) - sizeof(size_t)) & ~15;
		if (size > TINY_TRESHOLD)
			size = TINY_TRESHOLD;
	}
	topchunk->size_n_bits = (size | PREVINUSE);
	((struct s_alloc_chunk*)((char*)topchunk + size))->size_n_bits = PREVINUSE;
	malloc_struct.t_topchunk = (char*)topchunk + size + sizeof(size_t);
	printf("current top chunk:%ld\n", (char*)malloc_struct.t_topchunk - (char*)malloc_struct.tinyarenalist);
	return (topchunk);
}

//problems: if size is almost small, size + 32 is small
struct s_alloc_chunk	*new_tarena(const size_t size)
{
	struct s_arena		*new_arena;
	struct s_binlist	*last_chunk;

	printf("new arena !\n");
	new_arena =  mmap(NULL, TINY_ARENA_SZ, PROT_READ | PROT_WRITE,
								MAP_ANON | MAP_PRIVATE, -1, 0);
	if (new_arena == (void*)-1)
		return(NULL);
	last_chunk = (struct s_binlist*)((char*)malloc_struct.t_topchunk - sizeof(size_t));
	last_chunk->size_n_bits = ((size_t)((char*)malloc_struct.tinyarenalist + TINY_ARENA_SZ - (char*)last_chunk) - sizeof(size_t)) & ~15;
	if (last_chunk->size_n_bits >= 32)
	{
		last_chunk->size_n_bits |= PREVINUSE;
		add_tinybin(last_chunk);
	}
	new_arena->prev = malloc_struct.tinyarenalist;
	malloc_struct.tinyarenalist = new_arena;
	((struct s_alloc_chunk*)new_arena)->size_n_bits = size | PREVINUSE;
	((struct s_alloc_chunk*)((char*)new_arena + size))->size_n_bits = PREVINUSE;
	malloc_struct.t_topchunk = (char*)new_arena + size + sizeof(size_t);
	return((struct s_alloc_chunk*)new_arena);
}

void		*tiny_malloc(const size_t size)
{
	unsigned int			index;
	struct s_alloc_chunk	*ret;

	index = (size >> 4) - 2; // (size - 32) / 16
	ret = NULL;
	if (index < (FASTBIN_MAX >> 4) - 1)
		ret = check_fastbin(index);
	if (ret != NULL)
		return ((void*)((char*)ret + HEADER_SIZE));
	ret = check_tinybin(index);
	if (ret != NULL)
		return ((void*)((char*)ret + HEADER_SIZE));
	ret = do_fastbin(size);
	if (ret != NULL)
		return ((void*)((char*)ret + HEADER_SIZE));
	ret = get_from_ttopchunk(size);
	if (ret != NULL)
		return ((void*)((char*)ret + HEADER_SIZE));
	ret = new_tarena(size);
	if (ret != NULL)
		return ((void*)((char*)ret + HEADER_SIZE));
	return (NULL);
}

bool		malloc_init(void)
{
	int					i;
	struct s_binlist	*first_bin;

	malloc_struct.tinyarenalist =  mmap(NULL, TINY_ARENA_SZ, PROT_READ | PROT_WRITE,
									MAP_ANON | MAP_PRIVATE, -1, 0);
	if (malloc_struct.tinyarenalist == (void*)-1)
		return(false);
	malloc_struct.t_topchunk = (char*)malloc_struct.tinyarenalist + sizeof(struct s_arena);
	malloc_struct.smallarenalist =  mmap(NULL, SMALL_ARENA_SZ, PROT_READ | PROT_WRITE,
									MAP_ANON | MAP_PRIVATE, -1, 0);
	if (malloc_struct.smallarenalist == (void*)-1)
		return(false);
	malloc_struct.s_topchunk = (char*)malloc_struct.smallarenalist + sizeof(struct s_arena);
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

void		*malloc(size_t size)
{
	if (size >= ULONG_MAX - getpagesize() - HEADER_SIZE || size == 0)
		return (NULL);
	if (malloc_struct.bin[0] == NULL)
		if (malloc_init() == false)
			return (NULL);
	// round up to the next mult8 and not mult16 coz prevsize can hold datas
	if (size <= 24)
		size = 32;
	else
		size = (NEXT_8MULT(size) | 8) + sizeof(size_t);
	printf("size : %zu\n", size);
	if (size > SMALL_TRESHOLD)
		return (large_malloc(size));
	if (size > TINY_TRESHOLD)
		return (small_malloc(size));
	return (tiny_malloc(size));
}

