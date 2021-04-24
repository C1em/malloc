/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:57 by coremart          #+#    #+#             */
/*   Updated: 2021/04/18 13:15:54 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "malloc.h"
#include <stdlib.h>


# define TEST_ARENA_SZ 256

struct s_binlist *generate_chunk(size_t size_n_bits,
	struct s_binlist *next, struct s_binlist *prev) {

	static char	chunk_addr[TEST_ARENA_SZ] = { (char)0 };
	static struct s_binlist	*current_addr = (struct s_binlist*)chunk_addr;
	struct s_binlist		*ret;

	if (size_n_bits < 32) {

		printf("The size is too small: %zu\n", size_n_bits);
		exit(1);
	}
	if ((char*)current_addr + (size_n_bits & CHUNK_SIZE) + sizeof(void*) > chunk_addr + TEST_ARENA_SZ) {

		printf("You put a too large size, max size : %zd\n", (size_t)current_addr - (size_t)(chunk_addr + TEST_ARENA_SZ));
		exit(1);
	}

	current_addr->size_n_bits = size_n_bits;
	current_addr->next = next;
	current_addr->prev = prev;
	ret = current_addr;
	current_addr = (struct s_binlist*)((char*)chunk_addr + (size_n_bits & CHUNK_SIZE));
	current_addr->prevsize = size_n_bits & CHUNK_SIZE;
	return (ret);
}

void print_chunk(struct s_binlist *binlist)
{
	printf("------------------------\n");
	printf("prevsize:\t%zu\n", binlist->prevsize);
	printf("size:\t\t%zu\n", binlist->size_n_bits);
	printf("next:\t\t%p\n", binlist->next);
	printf("prev:\t\t%p\n", binlist->prev);
	printf("------------------------\n");
}

void print_allocchunk(struct s_alloc_chunk *chunkptr)
{
	printf("------------------------\n");
	printf("prevsize:\t%zu\n", chunkptr->prevsize);
	printf("size:\t\t%zu\n", chunkptr->size_n_bits & CHUNK_SIZE);
	printf("previnuse:\t%zu\n\n\n", chunkptr->size_n_bits & PREVINUSE);
	struct s_alloc_chunk	*nextchunk = (struct s_alloc_chunk*)((char*)chunkptr + (chunkptr->size_n_bits & CHUNK_SIZE));
	printf("previnuse:\t%zu\n\n\n", nextchunk->size_n_bits & PREVINUSE);
	printf("------------------------\n");
}

void random_test() {
	char	*arr[512];
	int		intarr[512];
	int		alloc_index = 0;
	int		free_index = 0;

	for (int i = 0; i < 512; i++) {

	if (arc4random() % 2) {

		arr[alloc_index] = (char*)malloc((intarr[alloc_index] = arc4random() % 504));
		for (int j = 0; j < intarr[alloc_index]; j++)
			arr[alloc_index][j] = -1;
		alloc_index++;
	}
	else if (free_index < alloc_index){
		free(arr[free_index]);
		free_index++;
	}
}
}

int		main(void)
{

	struct s_binlist* tmp = generate_chunk(32, NULL, NULL);
	struct s_binlist* tmp2 = generate_chunk(32, NULL, tmp);
	tmp->next = tmp2;
	print_chunk(tmp);
	print_chunk(tmp2);
	return (0);


}

// try ulong_max
