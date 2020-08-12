/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:26:54 by coremart          #+#    #+#             */
/*   Updated: 2020/08/12 19:10:47 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"


void		*malloc(size_t size)
{
	if (malloc_struct.arena == NULL)
		malloc_struct.arena = mmap(NULL, ARENA_SIZE, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);

}
