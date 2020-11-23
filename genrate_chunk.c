/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   genrate_chunk.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/11/21 12:58:29 by coremart          #+#    #+#             */
/*   Updated: 2020/11/21 20:07:46 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdio.h>
#include <stdlib.h>

extern __thread struct s_malloc_struct malloc_struct;

struct s_binlist *generate_chunk(size_t size_n_bits,
	struct s_binlist *next, struct s_binlist *prev, ssize_t pos) {

	static unsigned int		nb_chunk = 0;
	static struct s_binlist	*chunk_addr[256] = { NULL };
	struct s_binlist		*new_chunk_addr;

	pos++;
	if (chunk_addr[0] == NULL)
		chunk_addr[0] = mmap(NULL, PAGE_SZ, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, -1, 0);
	if (size_n_bits < 32) {

		printf("The size is too small: %zu\n", size_n_bits);
		exit(1);
	}
	if (pos > (ssize_t)nb_chunk) {

		printf("You put a too large position : %zd\n", pos);
		exit(1);
	}
	nb_chunk++;
	if (pos == 0)
		pos = nb_chunk;
	new_chunk_addr = (struct s_binlist*)((char*)chunk_addr[pos - 1] + (chunk_addr[pos - 1]->size_n_bits & CHUNK_SIZE));

	new_chunk_addr->prevsize = (chunk_addr[pos - 1]->size_n_bits & CHUNK_SIZE);
	new_chunk_addr->size_n_bits = size_n_bits;
	new_chunk_addr->next = next;
	new_chunk_addr->prev = prev;
	chunk_addr[pos] = new_chunk_addr;
	printf("final pos: %zd\nfinal nb_chunk: %u\nchunk_addr[0]: %p\nnew_chunk_addr %p\n", pos, nb_chunk, chunk_addr[0], new_chunk_addr);
	return (new_chunk_addr);
}


int		main(void) {

	generate_chunk(32, NULL, NULL, -1);
	generate_chunk(32, NULL, NULL, -1);
	return (0);
}
