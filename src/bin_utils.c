/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bin_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 15:22:50 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:34:09 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

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

struct s_alloc_chunk	*check_big_smallbin(size_t size) {

	struct s_binlist *bin = (struct s_binlist*)&malloc_struct.smallbin[(NB_SMALLBINS * 2) - 2 - 2];
	struct s_binlist *ret = bin;

	if (ret->prev == ret) // if list empty
		return (NULL);

	// get a big enough chunk
	ret = ret->prev;
	while (ret != bin && get_chunk_size(ret) - TINY_THRESHOLD < size)
		ret = ret->prev;

	if (ret == bin) // if we didn't find a chunk
		return (NULL);

	size_t remainder_sz = get_chunk_size(ret) - size;

	set_alloc_chunk_size(ret, size);

	struct s_binlist* remainder = (struct s_binlist*)get_next_chunk(ret);
	rm_bits(remainder, BITS);
	add_bits(remainder, PREVINUSE);
	set_freed_chunk_size(remainder, remainder_sz);

	// if small enough to be in smallbin
	if (get_chunk_size(remainder) < SMALL_THRESHOLD) {

		unlink_chunk(ret);
		add_smallbin(remainder);
	}
	else {

		remainder->next = ret->next;
		remainder->prev = ret->prev;
		remainder->prev->next = remainder;
		remainder->next->prev = remainder;
	}

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_smallbin(size_t size) {

	int index = get_smallbin_index(size);
	struct s_binlist *ret = (struct s_binlist*)&malloc_struct.smallbin[index - 2];

	if (ret->next == ret) // if list empty
		return (NULL);
	if (get_chunk_size(ret->next) < size) // if the biggest too small
		return (NULL);

	if (get_chunk_size(ret->prev) >= size)
		ret = ret->prev;
	else {

		while (get_chunk_size(ret->next) > size)
			ret = ret->next;
		if (get_chunk_size(ret->next) == size)
			ret = ret->next;
	}

	unlink_chunk(ret);
	add_bits(get_next_chunk(ret), PREVINUSE);

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_fastbin(size_t size) {

	if (size > FASTBIN_MAX)
		return (NULL);

	int index = (int)(size >> 4) - 2; // (size - 32) / 16
	struct s_fastbinlist *ret = malloc_struct.fastbin[index];

	if (ret == NULL) // if fastbin empty
		return (NULL);

	malloc_struct.fastbin[index] = malloc_struct.fastbin[index]->next;

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_tinybin(size_t size) {

	int index = (int)((size >> 5) << 1) - 2; // ((size - 32) / 32) * 2
	struct s_binlist *ret = (struct s_binlist*)&malloc_struct.tinybin[index - 2];

	if (ret->next == ret) // if list empty
		return (NULL);

	if (get_chunk_size(ret->prev) == size)
		ret = ret->prev;
	else if (get_chunk_size(ret->next) >= size)
		ret = ret->next;
	else
		return (NULL);

	unlink_chunk(ret);
	add_bits(get_next_chunk(ret), PREVINUSE);

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*check_big_tinybin(size_t size) {

	struct s_binlist *bin = (struct s_binlist*)&malloc_struct.tinybin[(NB_TINYBINS * 2) - 2 - 2];
	struct s_binlist *ret = bin;

	if (ret->prev == ret) // if list empty
		return (NULL);

	ret = ret->prev;
	while (ret != bin && get_chunk_size(ret) - TINY_MIN < size)
		ret = ret->prev;

	if (ret == bin)
		return (NULL);

	size_t remainder_sz = get_chunk_size(ret) - size;

	set_alloc_chunk_size(ret, size);

	struct s_binlist* remainder = (struct s_binlist*)get_next_chunk(ret);
	rm_bits(remainder, BITS);
	add_bits(remainder, PREVINUSE);
	set_freed_chunk_size(remainder, remainder_sz);

	// if small enough to be in tinybin
	if (get_chunk_size(remainder) < TINY_THRESHOLD) {

		unlink_chunk(ret);
		add_tinybin(remainder);
	}
	else {

		remainder->next = ret->next;
		remainder->prev = ret->prev;
		remainder->prev->next = remainder;
		remainder->next->prev = remainder;
	}

	return ((struct s_alloc_chunk*)ret);
}

struct s_alloc_chunk	*coalesce_fastbin(size_t size) {

	struct s_fastbinlist	*ret;

	// Iter through fastbins
	for (int i = 0; i < (FASTBIN_MAX >> 4) - 1; i++) {

		ret = malloc_struct.fastbin[i];
		while (ret != NULL) {

			// Unlink ret from fastbin
			malloc_struct.fastbin[i] = ret->next;

			ret = (struct s_fastbinlist*)coalesce_tinychunk((struct s_any_chunk*)ret);

			// If the coalesced chunk is large enough to be split
			if (get_chunk_size(ret) - TINY_MIN >= size) {

				return ((struct s_alloc_chunk*)split_tinychunk_for_size(
					(struct s_any_chunk*)ret,
					size
					));
			}

			add_tinybin((struct s_binlist*)ret);
			ret = malloc_struct.fastbin[i];
		}
	}
	return (NULL);
}
