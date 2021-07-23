/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/15 22:41:18 by coremart          #+#    #+#             */
/*   Updated: 2021/07/22 17:24:59 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/mman.h>
#include "malloc.h"
#include <stdbool.h>

extern struct s_malloc_struct malloc_struct;

/*
**	add to fastbin list without changing metadata
*/
void				add_fastbin(struct s_alloc_chunk *chunk) {

	int index = (get_chunk_size(chunk) >> 4) - 2; // (size - 32) / 16
	((struct s_fastbinlist*)chunk)->next = malloc_struct.fastbin[index];
	((struct s_binlist*)chunk)->prev = NULL;
	malloc_struct.fastbin[index] = (struct s_fastbinlist*)chunk;
}

void			add_big_tinybin(struct s_binlist *chunk) {

	struct s_binlist *prev = (struct s_binlist*)&malloc_struct.tinybin[(NB_TINYBINS * 2) - 2 - 2];

	chunk->prev = prev;
	chunk->next = prev->next;
	prev->next->prev = chunk;
	prev->next = chunk;

	rm_bits(get_next_chunk(chunk), PREVINUSE);
}

void			add_big_smallbin(struct s_binlist *chunk) {

	struct s_binlist *prev = (struct s_binlist*)&malloc_struct.smallbin[(NB_SMALLBINS * 2) - 2 - 2];

	chunk->prev = prev;
	chunk->next = prev->next;
	prev->next->prev = chunk;
	prev->next = chunk;

	rm_bits(get_next_chunk(chunk), PREVINUSE);
}


int				get_smallbin_index(size_t sz) {

	if (sz < 1024)
		return (((sz - TINY_THRESHOLD) >> 6) << 1); // ((sz - TINY_THRESHOLD) / 64) * 2
	else if (sz < 2048)
		return (16 + (((sz - 1024) >> 7) << 1)); // 16 + ((sz - 1024) / 128) * 2
	else if (sz >= SMALL_THRESHOLD)
		return ((NB_SMALLBINS * 2) - 2);

	int step_arr[] = {
		256,
		512,
		1024,
		2048
		};

	int cur_index_arr[] = {
		32,
		48,
		56,
		60
	};

	int step = step_arr[((sz) / 2048) - 1];
	int cur_index = cur_index_arr[((sz) / 2048) - 1];

	return (cur_index + ((sz - (sz & ~2047)) / step) * 2); // ((sz - prev_2048_mult(sz)) / step) * 2
}

void			add_smallbin(struct s_binlist *chunk) {

	// if the chunk is just before the top chunk
	if (get_next_chunk(chunk) == malloc_struct.topchunk_smallarena) {

		malloc_struct.topchunk_smallarena = (struct s_any_chunk*)chunk;
		chunk->size_n_bits = (ISTOPCHUNK | PREVINUSE);
		return ;
	}


	int index = get_smallbin_index(get_chunk_size(chunk));

	// if index above max, use last index
	if (index >= NB_SMALLBINS * 2) {

		add_big_smallbin(chunk);
		return;
	}

	struct s_binlist *prev = (struct s_binlist*)&malloc_struct.smallbin[index - 2];
	struct s_binlist *next = prev->next;

	// if list non empty
	if (prev != next) {

		// if smaller or equal to smallest
		if (get_chunk_size(chunk) <= get_chunk_size(prev->prev))
			next = prev;
		else {

			while (get_chunk_size(chunk) < get_chunk_size(next))
				next = next->next;
		}

		prev = next->prev;
	}

	chunk->prev = prev;
	chunk->next = next;
	next->prev = chunk;
	prev->next = chunk;

	rm_bits(get_next_chunk(chunk), PREVINUSE);
}

void			add_tinybin(struct s_binlist *chunk) {

	// if the chunk is just before the top chunk
	if (get_next_chunk(chunk) == malloc_struct.topchunk_tinyarena) {

		malloc_struct.topchunk_tinyarena = (struct s_any_chunk*)chunk;
		chunk->size_n_bits = (ISTOPCHUNK | PREVINUSE);
		return ;
	}

	int index = (((int)get_chunk_size(chunk) >> 5) << 1) - 2; // (size - 32) / 16

	// if index above max, use last index
	if (index >= NB_TINYBINS * 2) {
		add_big_tinybin(chunk);
		return;
	}

	struct s_binlist *prev = (struct s_binlist*)&malloc_struct.tinybin[index - 2];
	struct s_binlist *next = prev->next;


	// if smaller or equal to smallest
	if (prev != next && get_chunk_size(chunk) <= get_chunk_size(prev->prev)) {

		next = prev;
		prev = prev->prev;
	}

	chunk->prev = prev;
	chunk->next = next;
	next->prev = chunk;
	prev->next = chunk;

	rm_bits(get_next_chunk(chunk), PREVINUSE);
}

void				unlink_chunk(struct s_binlist *chunk) {

	chunk->next->prev = chunk->prev;
	chunk->prev->next = chunk->next;
}

/*
**	Fusion neighbours free chunks with chunk_ptr
**	chunk_ptr is returned as a free chunk (but it's not in a bin)
*/
struct s_binlist	*coalesce_smallchunk(struct s_any_chunk *chunk_ptr) {

	size_t new_sz = get_chunk_size(chunk_ptr);

	struct s_binlist *next_chunk = (struct s_binlist*)get_next_chunk(chunk_ptr);

	if (!(get_bits(next_chunk) & ISTOPCHUNK)) {

		if (!(get_bits(get_next_chunk(next_chunk)) & PREVINUSE)) {

			new_sz += get_chunk_size(next_chunk);
			unlink_chunk(next_chunk);
		}
	}
	// if there is lost space at the end of the arena
	else if (get_chunk_size(next_chunk) > HEADER_SIZE) {

		size_t lost_space = get_chunk_size(next_chunk) - HEADER_SIZE;
		new_sz += lost_space;

		struct s_any_chunk* new_top_chunk = ((struct s_any_chunk*)ptr_offset(next_chunk, lost_space));
		*new_top_chunk = (struct s_any_chunk){.size_n_bits = (HEADER_SIZE | ISTOPCHUNK)};
	}

	if (!(get_bits(chunk_ptr) & PREVINUSE)) {

		struct s_binlist *prev_chunk = (struct s_binlist*)get_prev_chunk(chunk_ptr);
		new_sz += get_chunk_size(prev_chunk);
		unlink_chunk(prev_chunk);
		chunk_ptr = (struct s_any_chunk*)prev_chunk;
	}

	// previnuse is always set since it's coalesced
	chunk_ptr->size_n_bits = PREVINUSE;
	set_freed_chunk_size(chunk_ptr, new_sz);

	rm_bits(get_next_chunk(chunk_ptr), PREVINUSE);

	return ((struct s_binlist*)chunk_ptr);
}

/*
**	Fusion neighbours free chunks with chunk_ptr
**	chunk_ptr is returned as a free chunk (but it's not in a bin)
*/
struct s_binlist	*coalesce_tinychunk(struct s_any_chunk *chunk_ptr) {

	size_t new_sz = get_chunk_size(chunk_ptr);

	struct s_binlist *next_chunk = (struct s_binlist*)get_next_chunk(chunk_ptr);

	if (!(get_bits(next_chunk) & ISTOPCHUNK)) {

		if (!(get_bits(get_next_chunk(next_chunk)) & PREVINUSE)) {

			new_sz += get_chunk_size(next_chunk);
			unlink_chunk(next_chunk);
		}
	}
	// if there is lost space at the end of the arena
	else if (get_chunk_size(next_chunk) > HEADER_SIZE) {

		size_t lost_space = get_chunk_size(next_chunk) - HEADER_SIZE;
		new_sz += lost_space;

		struct s_any_chunk* new_top_chunk = ((struct s_any_chunk*)ptr_offset(next_chunk, lost_space));
		*new_top_chunk = (struct s_any_chunk){.size_n_bits = (HEADER_SIZE | ISTOPCHUNK)};
	}

	if (!(get_bits(chunk_ptr) & PREVINUSE)) {

		struct s_binlist *prev_chunk = (struct s_binlist*)get_prev_chunk(chunk_ptr);
		new_sz += get_chunk_size(prev_chunk);
		unlink_chunk(prev_chunk);
		chunk_ptr = (struct s_any_chunk*)prev_chunk;
	}

	// previnuse is always set since it's coalesced
	chunk_ptr->size_n_bits = PREVINUSE;
	set_freed_chunk_size(chunk_ptr, new_sz);

	rm_bits(get_next_chunk(chunk_ptr), PREVINUSE);

	return ((struct s_binlist*)chunk_ptr);
}

void				do_small(struct s_binlist *chunk) {

	chunk = coalesce_smallchunk((struct s_any_chunk*)chunk);
	add_smallbin(chunk);
}

void				do_tiny(struct s_binlist *chunk) {

	chunk = coalesce_tinychunk((struct s_any_chunk*)chunk);
	add_tinybin(chunk);
}

void	print_addr(void *addr) {

	char			base[] = "0123456789abcdef";
	char			output[19] = "0x0000000000000000";
	int				i = 17;
	unsigned long	ul_addr = (unsigned long)addr;

	while (ul_addr > 0) {

		output[i] = base[ul_addr % 16];
		ul_addr /= 16;
		i--;
	}

	write(1, output, 18);
}

bool				is_in_arena(struct s_alloc_chunk *chunk) {

	if ((void*)chunk >= (void*)malloc_struct.tinyarenalist && (void*)chunk <= ptr_offset(malloc_struct.topchunk_tinyarena, - TINY_MIN))
		return (true);

	struct s_arena	*cur_arena = malloc_struct.tinyarenalist->prev;

	while (cur_arena != NULL) {

		if ((void*)chunk >= (void*)cur_arena
		&& (void*)chunk <= ptr_offset(cur_arena, TINY_ARENA_SZ - TINY_MIN - (long)HEADER_SIZE))
			return (true);

		cur_arena = cur_arena->prev;
	}

	if ((void*)chunk >= (void*)malloc_struct.smallarenalist && (void*)chunk <= ptr_offset(malloc_struct.topchunk_smallarena, - TINY_MIN))
		return (true);


	cur_arena = malloc_struct.smallarenalist->prev;

	while (cur_arena != NULL) {

		if ((void*)chunk >= (void*)cur_arena
		&& (void*)chunk <= ptr_offset(cur_arena, SMALL_ARENA_SZ - TINY_THRESHOLD - (long)HEADER_SIZE))
			return (true);

		cur_arena = cur_arena->prev;
	}

	return (false);
}

bool				is_valid_chunk(struct s_alloc_chunk *chunk) {

	if (is_in_arena(chunk)) {

		if (get_chunk_size(chunk) < SMALL_THRESHOLD
		&& get_chunk_size(chunk) % 16 == 0
		&& (get_bits(get_next_chunk(chunk)) & PREVINUSE) == PREVINUSE)
			return (true);
	}
	// can be a large chunk
	else if ((size_t)chunk % PAGE_SZ == 0) {

		if (chunk->size_n_bits >= SMALL_THRESHOLD && chunk->size_n_bits % PAGE_SZ == 0)
			return (true);
	}

	return (false);
}

void				free(void *ptr) {

	write(1, "free(", 5);
	print_addr(ptr);
	write(1, ")\n", 2);

	// if null or ptr is not aligned or malloc not initialized
	if (ptr == NULL || (size_t)ptr % 16 != 0 || malloc_struct.tinybin[0] == NULL)
		return;

	struct s_alloc_chunk *chunk = (struct s_alloc_chunk*)ptr_offset(ptr, - (long)HEADER_SIZE);

	if (!is_valid_chunk(chunk))
		return;

	if (get_chunk_size(chunk) <= FASTBIN_MAX)
		add_fastbin(chunk);
	else if (get_chunk_size(chunk) >= SMALL_THRESHOLD)
		munmap((void*)chunk, get_chunk_size(chunk));
	else if (get_chunk_size(chunk) < TINY_THRESHOLD)
		do_tiny((struct s_binlist*)chunk);
	else
		do_small((struct s_binlist*)chunk);
	// check that ptr is in an arena
	// check that next->prev = chunkptr and prev->next = chunkptr before collapsing
	// change the previnuse and prevsize of next chunk
	// collapse freed bin
	// change mmap_threshold
}
