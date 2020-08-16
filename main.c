/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:57 by coremart          #+#    #+#             */
/*   Updated: 2020/08/16 02:45:52 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "malloc.h"


int		main(void)
{
	// malloc(2);
	printf("tiny arena : %d\nsmall arena : %d\n", TINY_ARENA, SMALL_ARENA);
	char *alloc = malloc(4096);
	free(alloc);
	return (0);
}
// try ulong_max
