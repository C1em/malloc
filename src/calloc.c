/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/22 17:26:01 by coremart          #+#    #+#             */
/*   Updated: 2021/07/25 12:30:22 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

#include <sys/types.h>

#include <limits.h>

void		print_size(size_t sz);


#define	wsize	sizeof(u_int)
#define	wmask	(wsize - 1)

void	*ft_memset(void *dst, int c, size_t length) {

	size_t t;
	u_int uint_c = (u_char)c;
	u_char *uchar_dst = dst;

	if (length < 3 * wsize) {

		while (length != 0) {

			*uchar_dst++ = c;
			length--;
		}

		return (dst);
	}

	uint_c = (uint_c << 16) | uint_c;

	// get the offset from alignement
	t = (int)uchar_dst & wmask;

	if (t != 0) { // if not aligned

		// get the number of bytes to add for dst to be aligned
		t = wsize - t;

		length -= t;

		while (t-- != 0)
			*uchar_dst++ = c;
	}

	t = length / wsize;
	while (t-- != 0) {

		*(u_int *)uchar_dst = uint_c;
		uchar_dst += wsize;
	}

	// mop up trailing bytes, if any
	t = length & wmask;
	while (t-- != 0)
		*uchar_dst++ = c;

	return (dst);
}

void		*calloc(size_t count, size_t size) {

	if (count == 0 || size == 0) {

		count = 1;
		size = 1;
	}

	// write(1, "calloc(", 7);
	// print_size(count);
	// write(1, ", ", 2);
	// print_size(size);
	// write(1, ")\n", 2);

	void* ptr = malloc(count * size);
	ptr = ft_memset(ptr, 0, count * size);

	return (ptr);
}
