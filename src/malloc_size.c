/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc_size.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/26 12:41:14 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:30:49 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

#ifdef __APPLE__
# include <AvailabilityMacros.h>
#endif

size_t		malloc_size(const void* ptr) {

	#ifdef DEBUG
	write(1, "malloc_size(", 12);
	print_addr((void*)ptr);
	write(1, "):\t", 3);
	#endif

	pthread_mutex_lock(&mutex);
	struct s_alloc_chunk *chunk = (struct s_alloc_chunk*)ptr_offset((void*)ptr, - (long)HEADER_SIZE);
	if (!is_valid_chunk(chunk)) {

		pthread_mutex_unlock(&mutex);
		return (0);
	}

	// the reamaining memory is not added to large chunk
	size_t alloc_chunk_remain_mem = (get_chunk_size(chunk) >= SMALL_THRESHOLD)
									 ? 0
									 : sizeof(size_t);

	// starting from macos 11.0 the Objective-C Runtime check
	// the ""memory integrity"" by checking the size of allocated structure
	// https://opensource.apple.com/source/objc4/objc4-818.2/runtime/objc-runtime-new.mm.auto.html
	#ifdef MAC_OS_VERSION_11_0
	if (get_chunk_size(chunk) == 48) {

		if (((size_t*)chunk)[2] == 0x0000000080080000UL
		|| ((size_t*)chunk)[2] == 0x0000000080080001UL
		|| ((size_t*)chunk)[2] == 0x00000000a0080001UL
		|| ((size_t*)chunk)[2] == 0x0000000280080000UL
		|| ((size_t*)chunk)[2] == 0x0000000280880000UL
		|| ((size_t*)chunk)[2] == 0x0000000000880000UL
		|| ((size_t*)chunk)[2] == 0x0000000080880000UL
		|| ((size_t*)chunk)[2] == 0x00000002a0080001UL
		|| ((size_t*)chunk)[2] == 0x0000000280080001UL) {

			pthread_mutex_unlock(&mutex);
			return (32);
		}
	}
	#endif

	#ifdef DEBUG
	print_size(get_chunk_size(chunk) - HEADER_SIZE + alloc_chunk_remain_mem);
	write(1, "\n", 1);
	#endif

	size_t size = get_chunk_size(chunk) - HEADER_SIZE + alloc_chunk_remain_mem;

	pthread_mutex_unlock(&mutex);

	return (size);
}

size_t		malloc_good_size(size_t size) {

	#ifdef DEBUG
	write(1, "malloc_good_size(", 17);
	print_size(size);
	write(1, "):\t", 3);
	#endif

	// the reamaining memory is not added to large chunk
	size_t alloc_chunk_remain_mem = (chunk_size_from_user_size(size) >= SMALL_THRESHOLD)
									 ? 0
									 : sizeof(size_t);

	return (chunk_size_from_user_size(size) - HEADER_SIZE + alloc_chunk_remain_mem);
}
