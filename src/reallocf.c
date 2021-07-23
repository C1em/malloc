/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reallocf.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/23 12:25:13 by coremart          #+#    #+#             */
/*   Updated: 2021/07/23 14:57:42 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

void*	reallocf(void *ptr, size_t size) {

	void *nptr = realloc(ptr, size);

	if (!nptr && ptr)
		free(ptr);

	return (nptr);
}
