/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 15:16:22 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:49:00 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <unistd.h>
#include <stdio.h>

extern struct s_malloc_struct malloc_struct;

void		print_size(size_t sz) {

	char	output[21] = "00000000000000000000";
	int		i = 19;

	while (sz > 0) {

		output[i] += (sz % 10);
		sz /= 10;
		i--;
	}

	write(1, &output[i + 1], 19 - i);
}


void	print_addr(void *addr) {

	char			base[] = "0123456789abcdef";
	char			output[19] = "0x0000000000000000";
	int				i = 17;
	unsigned long	ul_addr = (unsigned long)addr;

	while (ul_addr > 0) {

		output[i] = base[ul_addr % 16];
		ul_addr /= 16;
		i--;
	}

	write(1, output, 18);
}


void	print_alloc_chunk(struct s_any_chunk *chunkptr) {

	printf(
		"0x%lX - 0x%lX : %zu octets\n",
		(size_t)chunkptr & 0x00000000000FFFFFUL,
		(size_t)ptr_offset(chunkptr, get_chunk_size(chunkptr)) & 0x00000000000FFFFFUL,
		get_chunk_size(chunkptr)
		);
}

size_t	print_tinyarenas(struct s_arena *tinyarenalist) {

	size_t total = 0;
	// stop condition
	if (tinyarenalist == NULL)
		return (0);

	// recursion
	total += print_tinyarenas(tinyarenalist->prev);

	// actual print
	printf("TINY : 0x%lX\n", ((size_t)tinyarenalist & 0x00000000000FFFFFUL));

	struct s_any_chunk	*cur_chunk = (struct s_any_chunk*)tinyarenalist;

	void*	last_chunk;
	if (tinyarenalist == malloc_struct.tinyarenalist)
		last_chunk = (void*)malloc_struct.topchunk_tinyarena;
	else
		last_chunk = (void*)ptr_offset(tinyarenalist, (long)TINY_ARENA_SZ - (long)HEADER_SIZE);


	while ((void*)cur_chunk < last_chunk) {

		if (get_bits(get_next_chunk(cur_chunk)) & PREVINUSE) {

			total += get_chunk_size(cur_chunk);
			print_alloc_chunk(cur_chunk);
		}

		cur_chunk = get_next_chunk(cur_chunk);
	}
	return (total);
}

size_t	print_smallarenas(struct s_arena *smallarenalist) {

	size_t total = 0;
	// stop condition
	if (smallarenalist == NULL)
		return (0);

	// recursion
	total += print_smallarenas(smallarenalist->prev);

	// actual print
	printf("SMALL : 0x%lX\n", ((size_t)smallarenalist & 0x00000000000FFFFFUL));

	struct s_any_chunk	*cur_chunk = (struct s_any_chunk*)smallarenalist;

	void*	last_chunk;
	if (smallarenalist == malloc_struct.smallarenalist)
		last_chunk = (void*)malloc_struct.topchunk_smallarena;
	else
		last_chunk = (void*)ptr_offset(smallarenalist, (long)SMALL_ARENA_SZ - (long)HEADER_SIZE);


	while ((void*)cur_chunk < last_chunk) {

		if (get_bits(get_next_chunk(cur_chunk)) & PREVINUSE) {

			total += get_chunk_size(cur_chunk);
			print_alloc_chunk(cur_chunk);
		}

		cur_chunk = get_next_chunk(cur_chunk);
	}
	return (total);
}

size_t	print_largearenas(struct s_arena *largearenalist) {

	size_t total = 0;
	// stop condition
	if (largearenalist == NULL)
		return (0);

	// recursion
	total += print_largearenas(largearenalist->prev);

	// actual print
	printf("LARGE : 0x%lX\n", ((size_t)largearenalist & 0x00000000000FFFFFUL));
	print_alloc_chunk((struct s_any_chunk*)largearenalist);

	return (total + get_chunk_size((struct s_any_chunk*)largearenalist));
}
