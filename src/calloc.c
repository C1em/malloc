/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/22 17:26:01 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:20:42 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <sys/types.h>

#define	wsize	sizeof(u_int)
#define	wmask	(wsize - 1)

void	*calloc_memset(void *dst, size_t length) {

	size_t t;
	u_char *uchar_dst = dst;

	if (length < 3 * wsize) {

		while (length != 0) {

			*uchar_dst++ = 0;
			length--;
		}

		return (dst);
	}

	t = length / wsize;
	while (t-- != 0) {

		*(u_int *)uchar_dst = 0;
		uchar_dst += wsize;
	}

	// mop up trailing bytes, if any
	t = length & wmask;
	while (t-- != 0)
		*uchar_dst++ = 0;

	return (dst);
}

void		*calloc(size_t count, size_t size) {

	if (count == 0 || size == 0)
		count = size = 1;

	#ifdef DEBUG
	write(1, "calloc(", 7);
	print_size(count);
	write(1, ", ", 2);
	print_size(size);
	write(1, ")\n", 2);
	#endif

	void* ptr = malloc(count * size);
	ptr = calloc_memset(ptr, count * size);

	return (ptr);
}
