/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_size.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/26 12:41:14 by coremart          #+#    #+#             */
/*   Updated: 2021/07/26 13:08:29 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

size_t		malloc_size(const void* ptr) {

	// write(1, "malloc_size\n", 12);
	struct s_alloc_chunk *chunk = (struct s_alloc_chunk*)ptr_offset((void*)ptr, - (long)HEADER_SIZE);
	if (!is_valid_chunk(chunk))
		return (0);

	return ((chunk->size_n_bits & CHUNK_SIZE) - HEADER_SIZE);
}

size_t		malloc_good_size(size_t size) {

	// write(1, "malloc_good_size\n", 17);
	return (chunk_size_from_user_size(size) - HEADER_SIZE);
}
