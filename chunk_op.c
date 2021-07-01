/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   chunk_op.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/04/24 12:50:29 by coremart          #+#    #+#             */
/*   Updated: 2021/06/30 22:25:26 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

extern inline unsigned int	get_bits(void *ptr) {

	return (((struct s_any_chunk*)ptr)->size_n_bits & BITS);
}

extern inline void			add_bits(void *ptr, unsigned int bits) {

	((struct s_any_chunk*)ptr)->size_n_bits |= bits;
}

extern inline void			rm_bits(void *ptr, unsigned int bits) {

	((struct s_any_chunk*)ptr)->size_n_bits &= ~bits;
}

extern inline size_t		get_chunk_size(void *ptr) {

	return (((struct s_any_chunk*)ptr)->size_n_bits & CHUNK_SIZE);
}

extern inline void			set_freed_chunk_size(void *ptr, size_t sz) {

	((struct s_any_chunk*)ptr)->size_n_bits = sz | (size_t)get_bits(ptr);
	get_next_chunk(ptr)->prevsize = sz;
}

extern inline void			set_alloc_chunk_size(void *ptr, size_t sz) {

	((struct s_any_chunk*)ptr)->size_n_bits = sz | (size_t)get_bits(ptr);
}

extern inline void			*ptr_offset(void *ptr, long offset) {

	return ((char*)ptr + offset);
}

extern inline struct s_any_chunk	*get_next_chunk(void* ptr) {

	return ((struct s_any_chunk*)ptr_offset(ptr, get_chunk_size(ptr)));
}

extern inline struct s_any_chunk	*get_prev_chunk(void* ptr) {

	return ((struct s_any_chunk*)ptr_offset(ptr, -(long)((struct s_any_chunk*)ptr)->prevsize));
}

/*
**	Example with user_size = 24:
**
**		chunk ->	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of previous chunk, if freed (8 bytes)        |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of chunk, in bytes (8 bytes)             |I|P|
**		mem->		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             User data (16 bytes)                              |
**					|                                                               |
**					|                                                               |
**					|                                                               |
**	nextchunk->		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of chunk, if freed (8 bytes)                 |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**
**	Note that the 8 first bytes of nextchunk are user data when it is allocated
*/
extern inline size_t		chunk_size_from_user_size(size_t user_size) {

	// min size is 32
	if (user_size <= 24)
		return (32);
	return ((NEXT_8MULT(user_size) | 8) + sizeof(size_t));
}
