/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:38:42 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"

extern struct s_malloc_struct malloc_struct;

void				do_small(struct s_binlist *chunk) {

	chunk = coalesce_smallchunk((struct s_any_chunk*)chunk);
	add_smallbin(chunk);
}

void				do_tiny(struct s_binlist *chunk) {

	chunk = coalesce_tinychunk((struct s_any_chunk*)chunk);
	add_tinybin(chunk);
}

void				free(void *ptr) {

	#ifdef DEBUG
	write(1, "free(", 5);
	print_addr(ptr);
	write(1, ")\n", 2);
	#endif

	pthread_mutex_lock(&mutex);

	// if null or ptr is not aligned or malloc not initialized
	if (ptr == NULL || (size_t)ptr % 16 != 0 || malloc_struct.tinybin[0] == NULL) {

		pthread_mutex_unlock(&mutex);
		return;
	}

	struct s_alloc_chunk *chunk = (struct s_alloc_chunk*)ptr_offset(ptr, - (long)HEADER_SIZE);

	if (!is_valid_chunk(chunk)) {

		pthread_mutex_unlock(&mutex);
		return;
	}

	if (get_chunk_size(chunk) <= FASTBIN_MAX)
		add_fastbin(chunk);
	else if (get_chunk_size(chunk) >= SMALL_THRESHOLD) {

		unlink_largearena((struct s_arena*)chunk);
		munmap((void*)chunk, get_chunk_size(chunk));
	}
	else if (get_chunk_size(chunk) < TINY_THRESHOLD)
		do_tiny((struct s_binlist*)chunk);
	else
		do_small((struct s_binlist*)chunk);

	pthread_mutex_unlock(&mutex);
}
