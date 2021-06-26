/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:57 by coremart          #+#    #+#             */
/*   Updated: 2021/06/26 03:43:12 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "malloc.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

# define TEST_ARENA_SZ 256

extern __thread struct s_malloc_struct malloc_struct;

bool	malloc_init(void);
struct s_alloc_chunk	*new_tinyarena(size_t size);

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

void		test_malloc_init(void) {

	malloc_init();
	if ((void*)malloc_struct.tinyarenalist != (void*)malloc_struct.topchunk_tinyarena)
		printf("malloc_struct.tinyarenalist != malloc_struct.topchunk_tinyarena\n");

	if ((void*)malloc_struct.smallarenalist != (void*)malloc_struct.topchunk_smallarena)
		printf("malloc_struct.smallarenalist != malloc_struct.topchunk_smallarena\n");

	struct s_any_chunk test = {.prevsize = 0, .size_n_bits = PREVINUSE | ISTOPCHUNK};

	if (memcmp(malloc_struct.tinyarenalist, &test, sizeof(struct s_any_chunk)))
		printf("memcmp(malloc_struct.tinyarenalist, &test, sizeof(struct s_any_chunk))\n");

	if (memcmp(malloc_struct.smallarenalist, &test, sizeof(struct s_any_chunk)))
		printf("memcmp(malloc_struct.smallarenalist, &test, sizeof(struct s_any_chunk))\n");

	struct s_binlist* bin;
	for (int i = 0; i < NB_TINYBINS; i++) {

		bin = (struct s_binlist*)&malloc_struct.tinybin[(i << 1) - 2];
		if (bin->next != bin)
			printf("bin->next != bin\n");

		if (bin->prev != bin)
			printf("bin->prev != bin\n");
	}

	if (((struct s_binlist*)&malloc_struct.tinybin[(NB_TINYBINS << 1) - 2])->next != NULL)
		printf("(struct s_binlist*)&malloc_struct.tinybin[(NB_TINYBINS << 1) - 2]->next != NULL\n");

	if (((struct s_binlist*)&malloc_struct.tinybin[(NB_TINYBINS << 1) - 2])->prev != NULL)
		printf("(struct s_binlist*)&malloc_struct.tinybin[(NB_TINYBINS << 1) - 2]->prev != NULL\n");

	for (int i = 0; i < NB_SMALLBINS; i++) {

		bin = (struct s_binlist*)(&(malloc_struct.smallbin[(i << 1) - 2]));
		if (bin->next != bin)
			printf("bin->next != bin\n");

		if (bin->prev != bin)
			printf("bin->prev != bin\n");
	}
}

void	test_new_tinyarena(void) {

	struct s_arena* prev_arena = malloc_struct.tinyarenalist;

	malloc_struct.topchunk_tinyarena = ptr_offset(malloc_struct.tinyarenalist, TINY_ARENA_SZ - 32);

	struct s_any_chunk* last_top_chunk = malloc_struct.topchunk_tinyarena;


	// test with not enough space at the end of arena
	struct s_alloc_chunk *new_arena = new_tinyarena(256);

	if ((void*)new_arena != (void*)malloc_struct.tinyarenalist)
		printf("1new_arena != malloc_struct.tinyarenalist\n");

	if ((void*)malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 256))
		printf("1malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 256)\n");

	if (malloc_struct.tinyarenalist->prev != prev_arena)
		printf("1malloc_struct.tinyarenalist->prev != prev_arena\n");

	if (last_top_chunk->size_n_bits != (32 | PREVINUSE | ISTOPCHUNK))
		printf("1last_top_chunk->size_n_bits != 32 | PREVINUSE | ISTOPCHUNK\n");

	if (((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (256 | PREVINUSE))
		printf("1((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != 256 | PREVINUSE");

	if (get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (PREVINUSE | ISTOPCHUNK))
		printf("1get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != PREVINUSE | ISTOPCHUNK");

	// test with no space at the end of arena
	prev_arena = malloc_struct.tinyarenalist;

	malloc_struct.topchunk_tinyarena = ptr_offset(malloc_struct.tinyarenalist, TINY_ARENA_SZ - HEADER_SIZE);

	last_top_chunk = malloc_struct.topchunk_tinyarena;
	new_arena = new_tinyarena(64);

	if ((void*)new_arena != (void*)malloc_struct.tinyarenalist)
		printf("2new_arena != malloc_struct.tinyarenalist\n");

	if ((void*)malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 64))
		printf("2malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 256)\n");

	if (malloc_struct.tinyarenalist->prev != prev_arena)
		printf("2malloc_struct.tinyarenalist->prev != prev_arena\n");

	if (last_top_chunk->size_n_bits != (HEADER_SIZE | PREVINUSE | ISTOPCHUNK))
		printf("2last_top_chunk->size_n_bits != 32 | PREVINUSE | ISTOPCHUNK\n");

	if (((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (64 | PREVINUSE))
		printf("2((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != 256 | PREVINUSE");

	if (get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (PREVINUSE | ISTOPCHUNK))
		printf("2get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != PREVINUSE | ISTOPCHUNK");

	// test with just the right space for a chunk at the end of arena
	prev_arena = malloc_struct.tinyarenalist;

	malloc_struct.topchunk_tinyarena = ptr_offset(malloc_struct.tinyarenalist, TINY_ARENA_SZ - 32 - HEADER_SIZE);

	last_top_chunk = malloc_struct.topchunk_tinyarena;
	new_arena = new_tinyarena(32);

	if ((void*)new_arena != (void*)malloc_struct.tinyarenalist)
		printf("3new_arena != malloc_struct.tinyarenalist\n");

	if ((void*)malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 32))
		printf("3malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 256)\n");

	if (malloc_struct.tinyarenalist->prev != prev_arena)
		printf("3malloc_struct.tinyarenalist->prev != prev_arena\n");

	if (last_top_chunk->size_n_bits != (32 | PREVINUSE))
		printf("3last_top_chunk->size_n_bits != 32 | PREVINUSE | ISTOPCHUNK\n");

	if (((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (32 | PREVINUSE))
		printf("3((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != 256 | PREVINUSE\n");

	if (get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (PREVINUSE | ISTOPCHUNK))
		printf("3get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != PREVINUSE | ISTOPCHUNK\n");

	if (get_next_chunk(last_top_chunk)->prevsize != 32)
		printf("3get_next_chunk(last_top_chunk)->prevsize != 32\n");

	if (get_next_chunk(last_top_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK))
		printf("3get_next_chunk(last_top_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK)\n");

	if ((void*)((struct s_binlist*)&malloc_struct.tinybin[-2])->next != (void*)last_top_chunk)
		printf("3((struct s_binlist*)&malloc_struct.tinybin[-2])->next != last_top_chunk\n");

	if ((void*)((struct s_binlist*)&malloc_struct.tinybin[-2])->prev != (void*)last_top_chunk)
		printf("3((struct s_binlist*)&malloc_struct.tinybin[-2])->prev != (char*)last_top_chunk + 4\n");

	// test with more than enough space at the end of the arena
	prev_arena = malloc_struct.tinyarenalist;

	malloc_struct.topchunk_tinyarena = ptr_offset(malloc_struct.tinyarenalist, TINY_ARENA_SZ - 256);

	last_top_chunk = malloc_struct.topchunk_tinyarena;
	new_arena = new_tinyarena(128);

	if ((void*)new_arena != (void*)malloc_struct.tinyarenalist)
		printf("4new_arena != malloc_struct.tinyarenalist\n");

	if ((void*)malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 128))
		printf("4malloc_struct.topchunk_tinyarena != ptr_offset(new_arena, 128)\n");

	if (malloc_struct.tinyarenalist->prev != prev_arena)
		printf("4malloc_struct.tinyarenalist->prev != prev_arena\n");

	if (last_top_chunk->size_n_bits != (256 - HEADER_SIZE | PREVINUSE))
		printf("4last_top_chunk->size_n_bits != 256 | PREVINUSE | ISTOPCHUNK\n");

	if (((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (128 | PREVINUSE))
		printf("4((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != 128 | PREVINUSE\n");

	if (get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != (PREVINUSE | ISTOPCHUNK))
		printf("4get_next_chunk((struct s_any_chunk*)malloc_struct.tinyarenalist)->size_n_bits != PREVINUSE | ISTOPCHUNK\n");

	if (get_next_chunk(last_top_chunk)->prevsize != 256 - HEADER_SIZE)
		printf("4get_next_chunk(last_top_chunk)->prevsize != 256\n");

	if (get_next_chunk(last_top_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK))
		printf("4get_next_chunk(last_top_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK)\n");

	if ((void*)((struct s_binlist*)&malloc_struct.tinybin[10])->next != (void*)last_top_chunk)
		printf("4((struct s_binlist*)&malloc_struct.tinybin[-2])->next != last_top_chunk\n");

	if ((void*)((struct s_binlist*)&malloc_struct.tinybin[10])->prev != (void*)last_top_chunk)
		printf("4((struct s_binlist*)&malloc_struct.tinybin[-2])->prev != (char*)last_top_chunk + 4\n");

}

int		main(void) {
	test_malloc_init();
	test_new_tinyarena();
	// struct s_binlist* tmp = generate_chunk(32, NULL, NULL);
	// struct s_binlist* tmp2 = generate_chunk(32, NULL, tmp);
	// tmp->next = tmp2;
	// print_chunk(tmp);
	// print_chunk(tmp2);
	// return (0);


}

// try ulong_max
