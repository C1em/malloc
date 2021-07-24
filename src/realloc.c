/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/22 12:08:30 by coremart          #+#    #+#             */
/*   Updated: 2021/07/22 13:41:07 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

void		*realloc(void *ptr, size_t size) {

	// realloc(NULL) is equivalent to malloc
	if (ptr == NULL)
		return (malloc(size));

	free(ptr);
	return (malloc(size));
}