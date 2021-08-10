/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/22 12:08:30 by coremart          #+#    #+#             */
/*   Updated: 2021/08/10 18:27:52 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <sys/types.h>

#define	wsize	sizeof(u_int)
#define	wmask	(wsize - 1)

void		*realloc_memcpy(void *dst, const void *src, size_t n) {

	size_t t;
	u_char *uchar_dst = dst;
	u_char *uchar_src = (u_char*)src;

	if (n < 3 * wsize) {

		while (n != 0) {

			*uchar_dst++ = *uchar_src++;
			n--;
		}

		return (dst);
	}

	t = n / wsize;
	while (t-- != 0) {

		*(u_int *)uchar_dst = *(u_int *)uchar_src;
		uchar_dst += wsize;
		uchar_src += wsize;
	}

	// mop up trailing bytes, if any
	t = n & wmask;
	while (t-- != 0)
		*uchar_dst++ = *uchar_src++;

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
	nptr = realloc_memcpy(nptr, ptr, copy_size);
	free(ptr);

	#ifdef DEBUG
	print_addr(nptr);
	write(1, "\n", 1);
	#endif

	return (nptr);
}
