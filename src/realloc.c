/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/22 12:08:30 by coremart          #+#    #+#             */
/*   Updated: 2021/07/26 14:18:37 by coremart         ###   ########.fr       */
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

void		*realloc(void *ptr, size_t size) {

	// write(1, "realloc\n", 8);

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
	size_t copy_size = (size <= malloc_size(ptr) ? size : malloc_size(ptr));
	nptr = ft_memcpy(nptr, ptr, copy_size);
	free(ptr);

	return (nptr);
}
