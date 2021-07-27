/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/22 12:08:30 by coremart          #+#    #+#             */
/*   Updated: 2021/07/27 16:07:04 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

// TODO: opti by coping 4 bytes
void		*ft_memcpy(void *restrict dst, const void *restrict src, size_t n) {

	char *char_dst = dst;
	char *char_src = (char*)src;

	while (n-- > 0)
		*char_dst++ = *char_src++;

	return (dst);
}
void	print_addr(void *addr);
void		print_size(size_t sz);

void		*realloc(void *ptr, size_t size) {

	#ifdef DEBUG
	write(1, "realloc(", 8);
	print_addr(ptr);
	write(1, ", ", 2);
	print_size(size);
	write(1, "):\t", 3);
	#endif

	// realloc(NULL) is equivalent to malloc
	if (ptr == NULL)
		return (malloc(size));

	if (size == 0) {

		free(ptr);
		return (malloc(0));
	}

	void* nptr = malloc(size);
	if (nptr == NULL)
		return (ptr);

	// min of malloc_size(ptr) and size
	size_t copy_size = (size <= malloc_size(ptr)) ? size : malloc_size(ptr);
	nptr = ft_memcpy(nptr, ptr, copy_size);
	free(ptr);

	#ifdef DEBUG
	print_addr(nptr);
	write(1, "\n", 1);
	#endif

	return (nptr);
}
