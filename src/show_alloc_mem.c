/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/11 17:40:31 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:43:53 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"
#include <stdio.h>

extern struct s_malloc_struct malloc_struct;

void	show_alloc_mem(void) {

	size_t total = 0;

	pthread_mutex_lock(&mutex);

	if (malloc_struct.tinybin[0] == NULL)
		return;

	total += print_tinyarenas(malloc_struct.tinyarenalist);
	total += print_smallarenas(malloc_struct.smallarenalist);
	total += print_largearenas(malloc_struct.largearenalist);

	printf("Total : %zu octets\n", total);
	pthread_mutex_unlock(&mutex);

}
