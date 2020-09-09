/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:57 by coremart          #+#    #+#             */
/*   Updated: 2020/09/10 00:29:51 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "malloc.h"
#include <stdlib.h>

void print_chunk(struct s_binlist *binlist)
{
	printf("prevsize:\t%zu\n", binlist->prevsize);
	printf("size:\t\t%zu\n", binlist->size_n_bits);
	printf("next:\t\t%p\n", binlist->next);
	printf("prev:\t\t%p\n", binlist->prev);
}

void print_allocchunk(struct s_alloc_chunk *chunkptr)
{
	printf("prevsize:\t%zu\n", chunkptr->prevsize);
	printf("size:\t\t%zu\n", chunkptr->size_n_bits & CHUNK_SIZE);
	printf("previnuse:\t%zu\n\n\n", chunkptr->size_n_bits & PREVINUSE);
	struct s_alloc_chunk	*nextchunk = (struct s_alloc_chunk*)((char*)chunkptr + (chunkptr->size_n_bits & CHUNK_SIZE));
	printf("previnuse:\t%zu\n\n\n", nextchunk->size_n_bits & PREVINUSE);
}

// void	test_coalesece(void)
// {
// 	unsigned char		map[512];
// 	struct s_binlist	*test;

// //	basic test
// 	((size_t*)map)[0] = (size_t)0;
// 	((size_t*)map)[1] = (size_t)208 | PREVINUSE;
// 	(((struct s_binlist**)&map)[2]) = (struct s_binlist*)map;
// 	(((struct s_binlist**)&map)[3]) = (struct s_binlist*)map;

// 	((size_t*)&(map[208]))[0] = (size_t)208;
// 	((size_t*)&(map[208]))[1] = (size_t)96;
// 	((struct s_binlist**)&(map[208]))[2] = (struct s_binlist*)&(map[208]);
// 	((struct s_binlist**)&(map[208]))[3] = (struct s_binlist*)&(map[208]);

// 	((size_t*)&(map[304]))[0] = (size_t)96;
// 	((size_t*)&(map[304]))[1] = (size_t)32 | IS_LAST;
// 	((struct s_binlist**)&(map[304]))[2] = (struct s_binlist*)&(map[304]);
// 	((struct s_binlist**)&(map[304]))[3] = (struct s_binlist*)&(map[304]);

// 	((size_t*)&(map[336]))[0] = 32;
// 	((size_t*)&(map[336]))[1] = 0;

// 	test = coalesce_chunk((struct s_binlist*)&(map[208]));
// 	if (test != (struct s_binlist*)map || ((size_t*)&(map[336]))[0] != 336
// 	|| ((size_t*)&(map[336]))[1] != 0 || test->prevsize != 0 || test->size_n_bits != 339)
// 	{
// 		printf("test1:\n%p != %p || %zu != 336 || %zu != 0 || %zu != 339\n", test, map, ((size_t*)&(map[336]))[0], test->prevsize, test->size_n_bits);
// 		exit(0);
// 	}

// //	only backward
// 	((size_t*)&(map[208]))[0] = (size_t)0;
// 	((size_t*)&(map[208]))[1] = (size_t)96 | PREVINUSE;

// 	((size_t*)&(map[336]))[0] = 32;

// 	test = coalesce_chunk((struct s_binlist*)&(map[304]));
// 	if (test != (struct s_binlist*)&(map[208]) || ((size_t*)&(map[336]))[0] != 128
// 	|| ((size_t*)&(map[336]))[1] != 0 || test->prevsize != 0 || test->size_n_bits != 131)
// 	{
// 		printf("test2:\n%p != %p || %zu != 128 || %zu != 0 || %zu != 131\n", test, &map[208], ((size_t*)&(map[336]))[0], test->prevsize, test->size_n_bits);
// 		exit(0);
// 	}
// // same without previnuse
// 	((size_t*)&(map[208]))[0] = (size_t)0;
// 	((size_t*)&(map[208]))[1] = (size_t)96;

// 	((size_t*)&(map[336]))[0] = 32;

// 	test = coalesce_chunk((struct s_binlist*)&(map[304]));
// 	if (test != (struct s_binlist*)&(map[208]) || ((size_t*)&(map[336]))[0] != 128
// 	|| ((size_t*)&(map[336]))[1] != 0 || test->prevsize != 0 || test->size_n_bits != 130)
// 	{
// 		printf("test3:\n%p != %p || %zu != 128 || %zu != 0 || %zu != 130\n", test, &map[208], ((size_t*)&(map[336]))[0], test->prevsize, test->size_n_bits);
// 		exit(0);
// 	}
// // next chunk in use
// 	((size_t*)map)[0] = (size_t)0;
// 	((size_t*)map)[1] = (size_t)208 | PREVINUSE;
// 	(((struct s_binlist**)&map)[2]) = (struct s_binlist*)map;
// 	(((struct s_binlist**)&map)[3]) = (struct s_binlist*)map;

// 	((size_t*)&(map[208]))[0] = (size_t)208;
// 	((size_t*)&(map[208]))[1] = (size_t)96;
// 	((struct s_binlist**)&(map[208]))[2] = (struct s_binlist*)&(map[208]);
// 	((struct s_binlist**)&(map[208]))[3] = (struct s_binlist*)&(map[208]);

// 	((size_t*)&(map[304]))[0] = (size_t)96;
// 	((size_t*)&(map[304]))[1] = (size_t)32;
// 	((struct s_binlist**)&(map[304]))[2] = (struct s_binlist*)17;
// 	((struct s_binlist**)&(map[304]))[3] = (struct s_binlist*)338;

// 	((size_t*)&(map[336]))[0] = 7896876;
// 	((size_t*)&(map[336]))[1] = PREVINUSE;

// 	test = coalesce_chunk((struct s_binlist*)&(map[208]));
// 	if (test != (struct s_binlist*)map || ((size_t*)&(map[304]))[0] != 304
// 	|| ((size_t*)&(map[336]))[1] != PREVINUSE || test->prevsize != 0 || test->size_n_bits != 305)
// 	{
// 		printf("test4:\n%p != %p || %zu != 304 || %zu != 0 || %zu != 305\n", test, map, ((size_t*)&(map[336]))[0], test->prevsize, test->size_n_bits);
// 		exit(0);
// 	}
// // only forward
// 	((size_t*)map)[0] = (size_t)0;
// 	((size_t*)map)[1] = (size_t)208 | PREVINUSE;
// 	(((struct s_binlist**)&map)[2]) = (struct s_binlist*)987987;
// 	(((struct s_binlist**)&map)[3]) = (struct s_binlist*)1769;

// 	((size_t*)&(map[208]))[0] = (size_t)3498798;
// 	((size_t*)&(map[208]))[1] = (size_t)96 | PREVINUSE;
// 	((struct s_binlist**)&(map[208]))[2] = (struct s_binlist*)&(map[208]);
// 	((struct s_binlist**)&(map[208]))[3] = (struct s_binlist*)&(map[208]);

// 	((size_t*)&(map[304]))[0] = (size_t)96;
// 	((size_t*)&(map[304]))[1] = (size_t)32;
// 	((struct s_binlist**)&(map[304]))[2] = (struct s_binlist*)&(map[304]);
// 	((struct s_binlist**)&(map[304]))[3] = (struct s_binlist*)&(map[304]);

// 	((size_t*)&(map[336]))[0] = 32;
// 	((size_t*)&(map[336]))[1] = 0;

// 	test = coalesce_chunk((struct s_binlist*)&(map[208]));
// 	if (test != (struct s_binlist*)&map[208] || ((size_t*)&(map[336]))[0] != 128
// 	|| ((size_t*)&(map[336]))[1] != 0 || test->prevsize != 3498798 || test->size_n_bits != 129)
// 	{
// 		printf("test5:\n%p != %p || %zu != 128 || %zu != 3498798 || %zu != 129\n", test, &map[336], ((size_t*)&(map[336]))[0], test->prevsize, test->size_n_bits);
// 		exit(0);
// 	}
// // no coalesce
// 	((size_t*)map)[0] = (size_t)0;
// 	((size_t*)map)[1] = (size_t)208 | PREVINUSE;
// 	(((struct s_binlist**)&map)[2]) = (struct s_binlist*)987987;
// 	(((struct s_binlist**)&map)[3]) = (struct s_binlist*)1769;

// 	((size_t*)&(map[208]))[0] = (size_t)3498798;
// 	((size_t*)&(map[208]))[1] = (size_t)96 | PREVINUSE | IS_LAST;
// 	((struct s_binlist**)&(map[208]))[2] = (struct s_binlist*)&(map[208]);
// 	((struct s_binlist**)&(map[208]))[3] = (struct s_binlist*)&(map[208]);

// 	((size_t*)&(map[304]))[0] = (size_t)96;
// 	((size_t*)&(map[304]))[1] = (size_t)32;
// 	((struct s_binlist**)&(map[304]))[2] = (struct s_binlist*)&(map[304]);
// 	((struct s_binlist**)&(map[304]))[3] = (struct s_binlist*)&(map[304]);

// 	((size_t*)&(map[336]))[0] = 32;
// 	((size_t*)&(map[336]))[1] = 0;

// 	test = coalesce_chunk((struct s_binlist*)&(map[208]));
// 	if (test != (struct s_binlist*)&map[208] || ((size_t*)&(map[304]))[0] != 96
// 	|| ((size_t*)&(map[304]))[1] != 32 || test->prevsize != 3498798 || test->size_n_bits != 99)
// 	{
// 		printf("test5:\n%p != %p || %zu != 96 || %zu != 3498798 || %zu != 99\n", test, &map[208], ((size_t*)&(map[304]))[0], test->prevsize, test->size_n_bits);
// 		exit(0);
// 	}
// // same but next chunk in use
// 	((size_t*)map)[0] = (size_t)0;
// 	((size_t*)map)[1] = (size_t)208 | PREVINUSE;
// 	(((struct s_binlist**)&map)[2]) = (struct s_binlist*)987987;
// 	(((struct s_binlist**)&map)[3]) = (struct s_binlist*)1769;

// 	((size_t*)&(map[208]))[0] = (size_t)3498798;
// 	((size_t*)&(map[208]))[1] = (size_t)96 | PREVINUSE;
// 	((struct s_binlist**)&(map[208]))[2] = (struct s_binlist*)&(map[208]);
// 	((struct s_binlist**)&(map[208]))[3] = (struct s_binlist*)&(map[208]);

// 	((size_t*)&(map[304]))[0] = (size_t)96;
// 	((size_t*)&(map[304]))[1] = (size_t)32;
// 	((struct s_binlist**)&(map[304]))[2] = (struct s_binlist*)412342;
// 	((struct s_binlist**)&(map[304]))[3] = (struct s_binlist*)42314123;

// 	((size_t*)&(map[336]))[0] = 532452;
// 	((size_t*)&(map[336]))[1] = PREVINUSE;

// 	test = coalesce_chunk((struct s_binlist*)&(map[208]));
// 	if (test != (struct s_binlist*)&map[208] || ((size_t*)&(map[304]))[0] != 96
// 	|| ((size_t*)&(map[304]))[1] != 32 || test->prevsize != 3498798 || test->size_n_bits != 97)
// 	{
// 		printf("test5:\n%p != %p || %zu != 96 || %zu != 3498798 || %zu != 97\n", test, &map[208], ((size_t*)&(map[304]))[0], test->prevsize, test->size_n_bits);
// 		exit(0);
// 	}
// }


int		main(void)
{
	char	*arr[512];
	int		intarr[512];
	// test_coalesece();
	for (int i = 0; i < 512; i++)
	{
		arr[i] = (char*)malloc((intarr[i] = rand() % 504));
		for (int j = 0; j < intarr[i]; j++)
			arr[i][j] = -1;
	}
	for (int i = 0; i < 512; i++)
	{
		free(arr[i]);
	}
	return (0);
}
// try ulong_max
