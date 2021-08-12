/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coalesce_chunk.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 15:24:35 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:25:12 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

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
