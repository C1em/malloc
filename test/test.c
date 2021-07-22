/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:57 by coremart          #+#    #+#             */
/*   Updated: 2021/07/22 11:07:34 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include "malloc.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

# define TEST_ARENA_SZ 4096

extern struct s_malloc_struct malloc_struct;

bool	malloc_init(void);
struct s_alloc_chunk	*new_tinyarena(size_t size);
struct s_alloc_chunk	*get_from_tinytopchunk(size_t size);
void					add_fastbin(struct s_alloc_chunk *chunk);
struct s_alloc_chunk	*check_tinybin(size_t size);
struct s_binlist		*coalesce_tinychunk(struct s_any_chunk *chunk_ptr);
struct s_alloc_chunk	*coalesce_fastbin(size_t size);
int				get_smallbin_index(size_t sz);

static inline struct s_alloc_chunk	*check_fastbin(size_t size) {

	if (size > FASTBIN_MAX)
		return (NULL);

	int index = (int)(size >> 4) - 2; // (size - 32) / 16
	struct s_fastbinlist *ret = malloc_struct.fastbin[index];
	if (ret == NULL)
		return (NULL);
	malloc_struct.fastbin[index] = malloc_struct.fastbin[index]->next;

	return ((struct s_alloc_chunk*)ret);
}

struct s_binlist *generate_chunk(
	size_t size_n_bits,
	bool is_alloc
	) {

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

	if (is_alloc) {

		rm_bits(current_addr, BITS);
		add_bits(current_addr, size_n_bits & BITS);

		set_alloc_chunk_size(current_addr, size_n_bits);
	}
	// free
	else {

		rm_bits(current_addr, BITS);
		add_bits(current_addr, size_n_bits & BITS);

		set_freed_chunk_size(current_addr, size_n_bits & CHUNK_SIZE);
	}

	ret = current_addr;
	current_addr = (struct s_binlist*)get_next_chunk(current_addr);

	return (ret);
}

bool is_in_fastbin(struct s_any_chunk *chunkptr) {

	int index = (get_chunk_size(chunkptr) >> 4) - 2; // (size - 32) / 16
	if (index < 0 || index >= (FASTBIN_MAX >> 4) - 1)
		return false;
	struct s_fastbinlist* cur_chunk = malloc_struct.fastbin[index];

	while (cur_chunk != NULL) {

		if ((void*)cur_chunk == (void*)chunkptr)
			return true;

		cur_chunk = cur_chunk->next;
	}
	return false;
}

void print_any_chunk(struct s_any_chunk *chunkptr) {

	printf("------------------------\n");

	if (!(chunkptr->size_n_bits & PREVINUSE))
		printf("prevsize:\t%20zu\n", chunkptr->prevsize);

	printf("size:\t\t%20zu", chunkptr->size_n_bits & CHUNK_SIZE);
	if (chunkptr->size_n_bits & PREVINUSE)
		printf(" | P");
	if (chunkptr->size_n_bits & ISTOPCHUNK)
		printf(" | I");
	if (is_in_fastbin(chunkptr))
		printf(" | F");

	printf("\n");
}

void print_chunk(struct s_binlist *binlist)
{
	printf("--------------------------------------------\n");
	printf("prevsize:\t%zu\n", binlist->prevsize);
	printf("size:\t\t%zu\n", binlist->size_n_bits);
	printf("next:\t\t%p\n", binlist->next);
	printf("prev:\t\t%p\n", binlist->prev);
	printf("--------------------------------------------\n");
}

void print_allocchunk(struct s_alloc_chunk *chunkptr)
{
	printf("--------------------------------------------\n");
	printf("prevsize:\t%zu\n", chunkptr->prevsize);
	printf("size:\t\t%zu\n", chunkptr->size_n_bits & CHUNK_SIZE);
	printf("previnuse:\t%zu\n\n\n", chunkptr->size_n_bits & PREVINUSE);
	struct s_alloc_chunk	*nextchunk = (struct s_alloc_chunk*)((char*)chunkptr + (chunkptr->size_n_bits & CHUNK_SIZE));
	printf("previnuse:\t%zu\n\n\n", nextchunk->size_n_bits & PREVINUSE);
	printf("--------------------------------------------\n");
}

void	print_tinyarenas(struct s_arena *tinyarenalist) {

	// stop condition
	if (tinyarenalist == NULL)
		return;

	// recursion
	print_tinyarenas(tinyarenalist->prev);

	// actual print
	struct s_any_chunk	*cur_chunk = (struct s_any_chunk*)tinyarenalist;

	printf("\nNEW ARENA : %p\n", tinyarenalist);

	void*	last_chunk;
	if (tinyarenalist == malloc_struct.tinyarenalist)
		last_chunk = (void*)malloc_struct.topchunk_tinyarena;
	else
		last_chunk = (void*)ptr_offset(tinyarenalist, (long)TINY_ARENA_SZ - (long)HEADER_SIZE);


	while ((void*)cur_chunk < last_chunk) {

		print_any_chunk(cur_chunk);
		if (get_chunk_size(cur_chunk) == 0)
			exit(1);
		cur_chunk = get_next_chunk(cur_chunk);
	}

	if (cur_chunk == last_chunk)
		print_any_chunk(cur_chunk);
}

void ez_random_test() {

	char	*arr[512];
	int		size_arr[512];
	int		alloc_index = 0;
	int		free_index = 0;

	for (int i = 0; i < 512; i++) {

		if (arc4random() % 2) {

			size_arr[alloc_index] = arc4random() % 488;
			arr[alloc_index] = (char*)malloc(size_arr[alloc_index]);

			// fill allocated mem with 0xff
			for (int j = 0; j < size_arr[alloc_index]; j++)
				arr[alloc_index][j] = -1;

			alloc_index++;
			printf("alloc_index: %d\n", alloc_index);
			print_tinyarenas(malloc_struct.tinyarenalist);
		}
		// if something to free
		else if (free_index < alloc_index){
			free(arr[free_index]);
			free_index++;

			print_tinyarenas(malloc_struct.tinyarenalist);
		}
	}
}

struct	s_alloc
{
	void*			ptr;
	struct s_alloc	*next;
};


void hard_random_test() {

	char	*arr[8192];
	size_t	arr_sz = 0;
	size_t	cur_sz;

	for (int i = 0; i < 8192; i++) {

		if (arc4random() % 2) {

			cur_sz = arc4random() % 10232;
			arr[arr_sz] = (char*)malloc(cur_sz);

			// fill allocated mem with 0xff
			for (int j = 0; j < cur_sz; j++)
				arr[arr_sz][j] = -1;

			arr_sz++;

			printf("array size: %zu\n", arr_sz);
			print_tinyarenas(malloc_struct.tinyarenalist);
			printf("END PRINT ARENA\n\n");
		}
		// if something to free
		else if (arr_sz > 0 && arc4random() % 2) {

			size_t rd_idx = arc4random() % arr_sz;
			free(arr[rd_idx]);

			arr[rd_idx] = arr[arr_sz - 1];
			arr_sz--;

			print_tinyarenas(malloc_struct.tinyarenalist);
			printf("END PRINT ARENA\n\n");
		}
	}
	for (int i = 0; i < arr_sz; i++) {

		free(arr[i]);
		print_tinyarenas(malloc_struct.tinyarenalist);
		printf("END PRINT ARENA\n\n");
	}

}

void test1() {

	void* chunk1 = malloc(364);
	(void)malloc(99);
	free(chunk1);
	(void)malloc(381);
	print_tinyarenas(malloc_struct.tinyarenalist);
	printf("END PRINT ARENA\n\n");
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

void	test_get_from_tinytopchunk(void) {

	struct s_any_chunk* last_top_chunk = malloc_struct.topchunk_tinyarena;
	get_from_tinytopchunk(32);

	if ((void*)malloc_struct.topchunk_tinyarena != (void*)((char*)last_top_chunk + 32))
		printf("malloc_struct.topchunk_tinyarena != (char*)last_top_chunk + 32\n");

	if (last_top_chunk->size_n_bits != (32 | PREVINUSE))
		printf("last_top_chunk->size_n_bits != (32 | PREVINUSE)\n");

	if (get_next_chunk(last_top_chunk) != (void*)malloc_struct.topchunk_tinyarena)
		printf("get_next_chunk(last_top_chunk) != (void*)malloc_struct.topchunk_tinyarena\n");

	if (get_next_chunk(last_top_chunk)->size_n_bits != (PREVINUSE | ISTOPCHUNK))
		printf("get_next_chunk(last_top_chunk)->size_n_bits != (PREVINUSE | ISTOPCHUNK)\n");
}

void	test_check_fastbin(void) {

	struct s_alloc_chunk* chunk = (struct s_alloc_chunk*)generate_chunk(32 | PREVINUSE, true);
	add_fastbin(chunk);

	struct s_alloc_chunk* ret = check_fastbin(32);

	if (ret != chunk)
		printf("ret != chunk\n");

	if (malloc_struct.fastbin[0] != NULL)
		printf("malloc_struct.fastbin[0] != NULL\n");

	if (ret->size_n_bits != (32 | PREVINUSE))
		printf("ret->size_n_bits != (32 | PREVINUSE)\n");

	ret = check_fastbin(32);

	if (ret != NULL)
		printf("ret != NULL\n");

	chunk = (struct s_alloc_chunk*)generate_chunk(272 | PREVINUSE, true);
	add_fastbin(chunk);


	if (malloc_struct.fastbin[15] == NULL)
		printf("malloc_struct.fastbin[15] == NULL\n");

	ret = check_fastbin(272);

	if (ret != chunk)
		printf("ret != chunk\n");

	if (malloc_struct.fastbin[15] != NULL)
		printf("malloc_struct.fastbin[15] != NULL\n");

	if (ret->size_n_bits != (272 | PREVINUSE))
		printf("ret->size_n_bits != (272 | PREVINUSE)\n");

}
void	reset_bin(void) {

	struct s_binlist* bin;
	for (int i = 0; i < NB_TINYBINS; i++) {

		bin = (struct s_binlist*)&malloc_struct.tinybin[(i << 1) - 2];
		while (bin->next != bin)
			unlink_chunk(bin->prev);
	}

	for (int i = 0; i < NB_SMALLBINS; i++) {

		bin = (struct s_binlist*)(&(malloc_struct.smallbin[(i << 1) - 2]));
		while (bin->next != bin)
			unlink_chunk(bin->prev);
	}

	bzero(malloc_struct.fastbin, (FASTBIN_MAX >> 4) - 1);
}

void	test_check_tinybin(void) {

	reset_bin();
	struct s_binlist* chunk = generate_chunk(32 | PREVINUSE, true);
	add_tinybin(chunk);

	struct s_alloc_chunk* ret = check_tinybin(32);

	if (ret != (struct s_alloc_chunk*)chunk)
		printf("ret != chunk\n");

	if ((void*)malloc_struct.tinybin[0] != (void*)&malloc_struct.tinybin[-2])
		printf("malloc_struct.tinybin[0] != &malloc_struct.tinybin[-2]\n");

	if ((void*)malloc_struct.tinybin[1] != (void*)&malloc_struct.tinybin[-2])
		printf("malloc_struct.tinybin[1] != &malloc_struct.tinybin[-2]\n");

	if (ret->size_n_bits != (32 | PREVINUSE))
		printf("ret->size_n_bits != (32 | PREVINUSE)\n");

	ret = check_tinybin(32);

	if (ret != NULL)
		printf("2ret != NULL\n");

	chunk = generate_chunk(272 | PREVINUSE, true);
	add_tinybin(chunk);


	if ((void*)malloc_struct.tinybin[14] == (void*)&malloc_struct.tinybin[12])
		printf("malloc_struct.tinybin[14] == &malloc_struct.tinybin[12]\n");

	if ((void*)malloc_struct.tinybin[15] == (void*)&malloc_struct.tinybin[12])
		printf("malloc_struct.tinybin[15] == &malloc_struct.tinybin[12]\n");

	ret = check_tinybin(272);

	if (ret != (struct s_alloc_chunk*)chunk)
		printf("ret != chunk\n");

	if ((void*)malloc_struct.tinybin[14] != (void*)&malloc_struct.tinybin[12])
		printf("malloc_struct.tinybin[14] != &malloc_struct.tinybin[12]\n");

	if ((void*)malloc_struct.tinybin[15] != (void*)&malloc_struct.tinybin[12])
		printf("malloc_struct.tinybin[15] != &malloc_struct.tinybin[12]\n");

	if (ret->size_n_bits != (272 | PREVINUSE))
		printf("ret->size_n_bits != (272 | PREVINUSE)\n");


	chunk = generate_chunk(112 | PREVINUSE, true);
	add_tinybin(chunk);
	struct s_binlist* chunk2 = generate_chunk(112 | PREVINUSE, true);
	add_tinybin(chunk2);
	struct s_binlist* chunk3 = generate_chunk(112 | PREVINUSE, true);
	add_tinybin(chunk3);
	struct s_binlist* chunk4 = generate_chunk(112 | PREVINUSE, true);
	add_tinybin(chunk4);

	ret = check_tinybin(112);
	ret = check_tinybin(112);
	ret = check_tinybin(112);
	if ((void*)malloc_struct.tinybin[4] != (void*)chunk)
		printf("malloc_struct.tinybin[4] != chunk\n");

	if ((void*)malloc_struct.tinybin[5] != (void*)chunk)
		printf("malloc_struct.tinybin[5] != chunk\n");

	ret = check_tinybin(112);
	if ((void*)malloc_struct.tinybin[4] != (void*)&malloc_struct.tinybin[2])
		printf("malloc_struct.tinybin[4] != &malloc_struct.tinybin[2]\n");

	if ((void*)malloc_struct.tinybin[5] != (void*)&malloc_struct.tinybin[2])
		printf("malloc_struct.tinybin[5] != &malloc_struct.tinybin[2]\n");

}

void	test_coalesce_tinychunk(void) {

	// coalesce both prev and next chunk
	reset_bin();
	struct s_binlist* prev_chunk = generate_chunk(32 | PREVINUSE, false);
	struct s_any_chunk* to_coalesce_chunk = (struct s_any_chunk*)generate_chunk(32, true);
	struct s_binlist* next_chunk = generate_chunk(32 | PREVINUSE, false);

	add_tinybin(prev_chunk);
	add_tinybin(next_chunk);
	struct s_binlist* coalesced_chunk = coalesce_tinychunk(to_coalesce_chunk);

	if (coalesced_chunk->size_n_bits != (96 | PREVINUSE))
		printf("1coalesced_chunk->size_n_bits != (96 | PREVINUSE)\n");

	if (get_bits(get_next_chunk(coalesced_chunk)) != 0)
		printf("1get_bits(get_next_chunk(coalesced_chunk)) != 0\n");

	if (check_tinybin(32) != NULL)
		printf("1check_tinybin(32) != NULL\n");


	// coalesce next chunk
	prev_chunk = generate_chunk(48 | PREVINUSE, false);
	to_coalesce_chunk = (struct s_any_chunk*)generate_chunk(32 | PREVINUSE, true);
	next_chunk = generate_chunk(32 | PREVINUSE, false);

	add_tinybin(next_chunk);
	coalesced_chunk = coalesce_tinychunk(to_coalesce_chunk);

	if (coalesced_chunk->size_n_bits != (64 | PREVINUSE))
		printf("2coalesced_chunk->size_n_bits != (64 | PREVINUSE)\n");

	if (get_bits(get_next_chunk(coalesced_chunk)) != 0)
		printf("2get_bits(get_next_chunk(coalesced_chunk)) != 0\n");

	if (check_tinybin(32) != NULL)
		printf("2check_tinybin(32) != NULL\n");

	if (get_prev_chunk(coalesced_chunk)->size_n_bits != (48 | PREVINUSE))
		printf("2get_prev_chunk(coalesced_chunk)->size_n_bits != (48 | PREVINUSE)\n");

	// coalesce prev chunk
	prev_chunk = generate_chunk(48 | PREVINUSE, false);
	to_coalesce_chunk = (struct s_any_chunk*)generate_chunk(32, true);
	next_chunk = generate_chunk(32 | PREVINUSE, true);
	add_bits(get_next_chunk(next_chunk), PREVINUSE);

	add_tinybin(prev_chunk);
	coalesced_chunk = coalesce_tinychunk(to_coalesce_chunk);

	if (coalesced_chunk->size_n_bits != (80 | PREVINUSE))
		printf("3coalesced_chunk->size_n_bits != (80 | PREVINUSE)\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != 32)
		printf("3get_next_chunk(coalesced_chunk)->size_n_bits != 32\n");

	if (check_tinybin(48) != NULL)
		printf("3check_tinybin(48) != NULL\n");

	// coalesce none
	prev_chunk = generate_chunk(48 | PREVINUSE, false);
	to_coalesce_chunk = (struct s_any_chunk*)generate_chunk(32 | PREVINUSE, true);
	next_chunk = generate_chunk(32 | PREVINUSE, true);
	add_bits(get_next_chunk(next_chunk), PREVINUSE);

	coalesced_chunk = coalesce_tinychunk(to_coalesce_chunk);

	if (coalesced_chunk->size_n_bits != (32 | PREVINUSE))
		printf("4coalesced_chunk->size_n_bits != (32 | PREVINUSE)\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != 32)
		printf("4get_next_chunk(coalesced_chunk)->size_n_bits != 32\n");

	if (get_prev_chunk(coalesced_chunk)->size_n_bits != (48 | PREVINUSE))
		printf("4get_prev_chunk(coalesced_chunk)->size_n_bits != (48 | PREVINUSE)\n");

	// next chunk is top chunk
	prev_chunk = generate_chunk(48 | PREVINUSE, false);
	to_coalesce_chunk = (struct s_any_chunk*)generate_chunk(32, true);
	next_chunk = (struct s_binlist*)get_next_chunk(to_coalesce_chunk);
	next_chunk->size_n_bits = (HEADER_SIZE | PREVINUSE | ISTOPCHUNK);

	add_tinybin(prev_chunk);
	coalesced_chunk = coalesce_tinychunk(to_coalesce_chunk);

	if (coalesced_chunk->size_n_bits != (80 | PREVINUSE))
		printf("5coalesced_chunk->size_n_bits != (80 | PREVINUSE)\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK))
		printf("5get_next_chunk(coalesced_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK)\n");

	if (check_tinybin(48) != NULL)
		printf("5check_tinybin(32) != NULL\n");

	// next chunk is top chunk with free space
	prev_chunk = generate_chunk(48 | PREVINUSE, false);
	to_coalesce_chunk = (struct s_any_chunk*)generate_chunk(32, true);
	next_chunk = generate_chunk(32 | PREVINUSE | ISTOPCHUNK, true);

	add_tinybin(prev_chunk);
	add_tinybin(next_chunk);
	coalesced_chunk = coalesce_tinychunk(to_coalesce_chunk);

	if (coalesced_chunk->size_n_bits != (96 | PREVINUSE))
		printf("6coalesced_chunk->size_n_bits != (96 | PREVINUSE)\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK))
		printf("6get_next_chunk(coalesced_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK)\n");

	if (check_tinybin(48) != NULL)
		printf("6check_tinybin(32) != NULL\n");
}

void		test_split_tinychunk_for_size() {

	// just enough space to split
	struct s_binlist*	chunk = generate_chunk(64 | PREVINUSE, false);

	get_next_chunk(chunk)->size_n_bits = (ISTOPCHUNK | PREVINUSE);

	struct s_any_chunk*	new_chunk = split_tinychunk_for_size((struct s_any_chunk*)chunk, 32);

	if ((void*)chunk != (void*)new_chunk)
		printf("(void*)chunk != (void*)new_chunk\n");

	if (get_chunk_size(new_chunk) != 32)
		printf("get_chunk_size(new_chunk) != 32\n");

	if (get_chunk_size(get_next_chunk(new_chunk)) != 32)
		printf("get_chunk_size(get_next_chunk(new_chunk)) != 32\n");

	if (get_bits(new_chunk) != PREVINUSE)
		printf("get_bits(new_chunk) != PREVINUSE\n");

	if (get_bits(get_next_chunk(new_chunk)) != PREVINUSE)
		printf("get_bits(get_next_chunk(new_chunk)) != PREVINUSE\n");

	if (get_bits(get_next_chunk(get_next_chunk(new_chunk))) != ISTOPCHUNK)
		printf("get_bits(get_next_chunk(get_next_chunk(new_chunk))) != ISTOPCHUNK\n");

	if ((void*)check_tinybin(32) != (void*)get_next_chunk(new_chunk))
		printf("check_tinybin(32) != get_next_chunk(new_chunk)\n");


	// more than enough space to split
	chunk = generate_chunk(128 | PREVINUSE, false);
	get_next_chunk(chunk)->size_n_bits = (ISTOPCHUNK | PREVINUSE);

	new_chunk = split_tinychunk_for_size((struct s_any_chunk*)chunk, 32);

	if ((void*)chunk != (void*)new_chunk)
		printf("(void*)chunk != (void*)new_chunk\n");

	if (get_chunk_size(new_chunk) != 32)
		printf("get_chunk_size(new_chunk) != 32\n");

	if (get_chunk_size(get_next_chunk(new_chunk)) != 96)
		printf("get_chunk_size(get_next_chunk(new_chunk)) != 96\n");

	if (get_bits(new_chunk) != PREVINUSE)
		printf("get_bits(new_chunk) != PREVINUSE\n");

	if (get_bits(get_next_chunk(new_chunk)) != PREVINUSE)
		printf("get_bits(get_next_chunk(new_chunk)) != PREVINUSE\n");

	if (get_bits(get_next_chunk(get_next_chunk(new_chunk))) != ISTOPCHUNK)
		printf("get_bits(get_next_chunk(get_next_chunk(new_chunk))) != ISTOPCHUNK\n");

	if ((void*)check_tinybin(96) != (void*)get_next_chunk(new_chunk))
		printf("check_tinybin(96) != get_next_chunk(new_chunk)\n");

	// not enough space to split
	chunk = generate_chunk(128 | PREVINUSE, false);
	get_next_chunk(chunk)->size_n_bits = (ISTOPCHUNK | PREVINUSE);

	new_chunk = split_tinychunk_for_size((struct s_any_chunk*)chunk, 112);

	if ((void*)chunk != (void*)new_chunk)
		printf("(void*)chunk != (void*)new_chunk\n");

	if (get_chunk_size(new_chunk) != 128)
		printf("get_chunk_size(new_chunk) != 128\n");

	if (get_chunk_size(get_next_chunk(new_chunk)) != 0)
		printf("get_chunk_size(get_next_chunk(new_chunk)) != 0\n");

	if (get_bits(new_chunk) != PREVINUSE)
		printf("get_bits(new_chunk) != PREVINUSE\n");

	if (get_bits(get_next_chunk(new_chunk)) != (ISTOPCHUNK | PREVINUSE))
		printf("get_bits(get_next_chunk(new_chunk)) != (ISTOPCHUNK | PREVINUSE)\n");
}

void		test_coalesce_fastbin(void) {

	reset_bin();

	struct s_binlist* chunk1 = generate_chunk(32 | PREVINUSE, false);
	struct s_binlist* chunk2 = generate_chunk(32 | PREVINUSE, false);
	struct s_binlist* chunk3 = generate_chunk(32 | PREVINUSE, false);
	struct s_binlist* chunk4 = generate_chunk(32 | PREVINUSE, false);
	struct s_binlist* chunk5 = generate_chunk(32 | PREVINUSE, false);
	struct s_binlist* chunk6 = generate_chunk(32 | PREVINUSE, false);
	struct s_binlist* chunk7 = generate_chunk(128 | PREVINUSE, false);
	struct s_binlist* chunk8 = generate_chunk(32 | PREVINUSE, false);
	struct s_binlist* chunk9 = generate_chunk(256 | PREVINUSE, false);

	add_fastbin((struct s_alloc_chunk*)chunk1);
	add_fastbin((struct s_alloc_chunk*)chunk2);
	add_fastbin((struct s_alloc_chunk*)chunk3);
	add_fastbin((struct s_alloc_chunk*)chunk4);
	add_fastbin((struct s_alloc_chunk*)chunk5);

	add_fastbin((struct s_alloc_chunk*)chunk7);

	add_fastbin((struct s_alloc_chunk*)chunk9);

	get_next_chunk(chunk9)->size_n_bits = (HEADER_SIZE | ISTOPCHUNK | PREVINUSE);


	// test classical coalesce from chunk 1 to 5
	struct s_alloc_chunk* coalesced_chunk = coalesce_fastbin(160);

	if ((void*)coalesced_chunk != (void*)chunk1)
		printf("(void*)coalesced_chunk != (void*)chunk1\n");

	if (get_chunk_size(coalesced_chunk) != 160)
		printf("get_chunk_size(coalesced_chunk) != 160\n");

	if (get_bits(coalesced_chunk) != PREVINUSE)
		printf("get_bits(coalesced_chunk) != PREVINUSE\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != (32 | PREVINUSE))
		printf("get_next_chunk(coalesced_chunk)->size_n_bits != (32 | PREVINUSE)\n");

	if (check_fastbin(32) != NULL)
		printf("check_fastbin(32) != NULL\n");

	// test no coalesce from chunk 6 to 8
	coalesced_chunk = coalesce_fastbin(128);

	if ((void*)coalesced_chunk != (void*)chunk7)
		printf("(void*)coalesced_chunk != (void*)chunk7\n");

	if (get_chunk_size(coalesced_chunk) != 128)
		printf("get_chunk_size(coalesced_chunk) != 128\n");

	if (get_bits(coalesced_chunk) != PREVINUSE)
		printf("get_bits(coalesced_chunk) != PREVINUSE\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != (32 | PREVINUSE))
		printf("get_next_chunk(coalesced_chunk)->size_n_bits != (32 | PREVINUSE)\n");

	if (check_fastbin(128) != NULL)
		printf("check_fastbin(128) != NULL\n");

	// test next chunk ISTOPCHUNK

	coalesced_chunk = coalesce_fastbin(256);

	if ((void*)coalesced_chunk != (void*)chunk9)
		printf("(void*)coalesced_chunk != (void*)chunk9\n");

	if (get_chunk_size(coalesced_chunk) != 256)
		printf("get_chunk_size(coalesced_chunk) != 256\n");

	if (get_bits(coalesced_chunk) != PREVINUSE)
		printf("get_bits(coalesced_chunk) != PREVINUSE\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK | PREVINUSE))
		printf("get_next_chunk(coalesced_chunk)->size_n_bits != (HEADER_SIZE | ISTOPCHUNK | PREVINUSE)\n");

	if (check_fastbin(256) != NULL)
		printf("check_fastbin(256) != NULL\n");

	// test next chunk ISTOPCHUNK with space left (put space left in tinybin !!!!)
	struct s_binlist* chunk10 = generate_chunk(272 | PREVINUSE, false);
	struct s_binlist* chunk11 = generate_chunk(496 | ISTOPCHUNK | PREVINUSE, false);

	add_fastbin((struct s_alloc_chunk*)chunk10);
	add_tinybin(chunk11);

	coalesced_chunk = coalesce_fastbin(512);

	if ((void*)coalesced_chunk != (void*)chunk10)
		printf("(void*)coalesced_chunk != (void*)chunk10\n");

	if (get_chunk_size(coalesced_chunk) != 512)
		printf("get_chunk_size(coalesced_chunk) != 512\n");

	if (get_bits(coalesced_chunk) != PREVINUSE)
		printf("get_bits(coalesced_chunk) != PREVINUSE\n");

	if (get_next_chunk(coalesced_chunk)->size_n_bits != (240 | PREVINUSE))
		printf("get_next_chunk(coalesced_chunk)->size_n_bits != (240 | PREVINUSE)\n");

	if (check_fastbin(512) != NULL)
		printf("check_fastbin(512) != NULL\n");

	if (check_tinybin(496) != NULL)
		printf("check_tinybin(496) != NULL\n");

	if (get_next_chunk(get_next_chunk(coalesced_chunk))->size_n_bits != (HEADER_SIZE | ISTOPCHUNK))
		printf("get_next_chunk(get_next_chunk(coalesced_chunk))->size_n_bits != (HEADER_SIZE | ISTOPCHUNK)\n");


	if ((void*)check_tinybin(240) != (void*)get_next_chunk(coalesced_chunk))
		printf("(void*)check_tinybin(240) != (void*)get_next_chunk(coalesced_chunk)\n");

	if (get_next_chunk(get_next_chunk(coalesced_chunk))->size_n_bits != (HEADER_SIZE | ISTOPCHUNK | PREVINUSE))
		printf("get_next_chunk(get_next_chunk(coalesced_chunk))->size_n_bits != (HEADER_SIZE | ISTOPCHUNK | PREVINUSE)\n");
}


int		main(void) {
	// test_malloc_init();
	// test_new_tinyarena();
	// test_get_from_tinytopchunk();
	// test_check_fastbin();
	// test_check_tinybin();
	// test_coalesce_tinychunk();
	// test_split_tinychunk_for_size();
	// test_coalesce_fastbin();

	// ez_random_test();
	hard_random_test();

	// test1();
	// struct s_binlist* tmp = generate_chunk(32, NULL, NULL);
	// struct s_binlist* tmp2 = generate_chunk(32, NULL, tmp);
	// tmp->next = tmp2;
	// print_chunk(tmp);
	// print_chunk(tmp2);
	// return (0);


}

// try ulong_max
