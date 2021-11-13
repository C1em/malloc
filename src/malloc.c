/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:41:56 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <limits.h>
#include <pthread.h>
#include <sys/mman.h>

struct s_malloc_struct malloc_struct;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void		*large_malloc(size_t size) {

	void *alloc =  mmap(
		NULL,
		NEXT_PAGEALIGN(size),
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);

	if (alloc == (void*)-1)
		return (NULL);

	// add arena to list
	((struct s_arena*)alloc)->prev = malloc_struct.largearenalist;
	malloc_struct.largearenalist = (struct s_arena*)alloc;

	((struct s_alloc_chunk*)alloc)->size_n_bits = NEXT_PAGEALIGN(size);
	return (ptr_offset(alloc, (long)HEADER_SIZE));
}

struct s_alloc_chunk	*get_from_smalltopchunk(size_t size) {

	struct s_alloc_chunk *topchunk = (struct s_alloc_chunk*)malloc_struct.topchunk_smallarena;

	// If size is too big to fit in
	if ((size_t)topchunk + size + HEADER_SIZE > (size_t)malloc_struct.smallarenalist + SMALL_ARENA_SZ)
		return (NULL);

	set_alloc_chunk_size(topchunk, size);
	rm_bits(topchunk, ISTOPCHUNK);

	malloc_struct.topchunk_smallarena = get_next_chunk(malloc_struct.topchunk_smallarena);
	malloc_struct.topchunk_smallarena->size_n_bits = PREVINUSE | ISTOPCHUNK;

	return (topchunk);
}


void		*small_malloc(const size_t size) {

	struct s_alloc_chunk *(*malloc_strategy[4])(size_t) = {
		check_smallbin,
		check_big_smallbin,
		get_from_smalltopchunk,
		new_smallarena
	};

	struct s_alloc_chunk *ret = NULL;
	for (unsigned int i = 0; i < sizeof(malloc_strategy) / sizeof(malloc_strategy[0]); i++) {

		ret = malloc_strategy[i](size);
		if (ret != NULL)
			return (ptr_offset(ret, HEADER_SIZE));
	}

	return (NULL);
}

struct s_alloc_chunk	*get_from_tinytopchunk(size_t size) {

	struct s_alloc_chunk *topchunk = (struct s_alloc_chunk*)malloc_struct.topchunk_tinyarena;

	// If size is too big to fit in
	if ((size_t)topchunk + size + HEADER_SIZE > (size_t)malloc_struct.tinyarenalist + TINY_ARENA_SZ)
		return (NULL);

	set_alloc_chunk_size(topchunk, size);
	rm_bits(topchunk, ISTOPCHUNK);

	malloc_struct.topchunk_tinyarena = get_next_chunk(malloc_struct.topchunk_tinyarena);
	malloc_struct.topchunk_tinyarena->size_n_bits = PREVINUSE | ISTOPCHUNK;

	return (topchunk);
}

void		*tiny_malloc(size_t size) {

	struct s_alloc_chunk *(*malloc_strategy[6])(size_t) = {
		check_fastbin,
		check_tinybin,
		check_big_tinybin,
		coalesce_fastbin,
		get_from_tinytopchunk,
		new_tinyarena
	};

	struct s_alloc_chunk *ret = NULL;
	for (unsigned int i = 0; i < sizeof(malloc_strategy) / sizeof(malloc_strategy[0]); i++) {

		ret = malloc_strategy[i](size);
		if (ret != NULL)
			return (ptr_offset(ret, HEADER_SIZE));
	}
	return (NULL);
}
bool		malloc_init(void) {

	malloc_struct.tinyarenalist =  mmap(
		NULL,
		TINY_ARENA_SZ + SMALL_ARENA_SZ,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1,
		0
	);

	if (malloc_struct.tinyarenalist == (void*)-1)
		return(false);

	add_bits(malloc_struct.tinyarenalist, PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_tinyarena = (struct s_any_chunk*)malloc_struct.tinyarenalist;

	malloc_struct.smallarenalist = ptr_offset(malloc_struct.tinyarenalist, TINY_ARENA_SZ);

	if (malloc_struct.smallarenalist == (void*)-1)
		return (false);

	add_bits(malloc_struct.smallarenalist, PREVINUSE | ISTOPCHUNK);
	malloc_struct.topchunk_smallarena = (struct s_any_chunk*)malloc_struct.smallarenalist;

	// Each bin is a looping list where bin[0] is the next elem and bin[1] is the prev elem
	// then bin[2] is the next elem and bin[3] is the prev
	struct s_binlist *init_bin;
	for (int i = 0; i < NB_SMALLBINS + NB_TINYBINS; i++) {

		init_bin = (struct s_binlist*)(&(malloc_struct.smallbin[(i << 1) - 2]));
		init_bin->next = init_bin;
		init_bin->prev = init_bin;
	}

	return (true);
}

void		*malloc(size_t size) {

	#ifdef DEBUG
	write(1, "malloc(", 7);
	print_size(size);
	write(1, "):\t", 3);
	#endif

	pthread_mutex_lock(&mutex);
	if (size >= ULONG_MAX - PAGE_SZ - HEADER_SIZE) {

		pthread_mutex_unlock(&mutex);
		return (NULL);
	}

	if (malloc_struct.tinybin[0] == NULL)
		if (malloc_init() == false) {

			pthread_mutex_unlock(&mutex);
			return (NULL);
		}


	size_t chunk_size =  chunk_size_from_user_size(size);

	void	*res = NULL;
	if (chunk_size >= SMALL_THRESHOLD)
		res = large_malloc(chunk_size);
	else if (chunk_size >= TINY_THRESHOLD)
		res = small_malloc(chunk_size);
	else
		res = tiny_malloc(chunk_size);

	#ifdef DEBUG
	print_addr(res);
	write(1, "\n", 1);
	#endif

	pthread_mutex_unlock(&mutex);
	return (res);
}
