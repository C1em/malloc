/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:04 by coremart          #+#    #+#             */
/*   Updated: 2021/06/18 16:37:46 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

# define HEADER_SIZE			sizeof(struct s_any_chunk)

# define TINY_TRESHOLD			512
# define SMALL_TRESHOLD			2048
# define FASTBIN_MAX			272

# define PAGE_SZ				getpagesize()
# define NEXT_8MULT(x)			((7 + x) & ~7)
# define NEXT_16MULT(x)			((15 + x) & ~15)
# define NEXT_PAGEALIGN(x)		((PAGE_SZ - 1 + x) & ~(PAGE_SZ - 1))
# define NEXT_PW2(x)			(1 << (32 - __builtin_clz(x - 1)))

# define TINY_ARENA_SZ			NEXT_PAGEALIGN((TINY_TRESHOLD + HEADER_SIZE) * 100)
# define SMALL_ARENA_SZ			NEXT_PAGEALIGN((SMALL_TRESHOLD + HEADER_SIZE) * 100)

# define PREVINUSE				0x1
# define ISTOPCHUNK				0x2 // the top chunk of an arena
# define BITS					0x7
# define CHUNK_SIZE				~0x7UL

# define NBINS					128

/*
** Freed chunk:
**
**	chunk ->		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of previous chunk, if freed (8 bytes)        |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of chunk, in bytes (8 bytes)             |I|P|
**	mem ->			+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Forward pointer to next chunk in list (8 bytes)   |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Back pointer to previous chunk in list (8 bytes)  |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Unused space (can be 0 bytes long)                |
**					|                                                               |
**					|                                                               |
**	nextchunk ->	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of chunk, in bytes (8 bytes)                 |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**
*/
struct s_binlist {

	size_t				prevsize;
	size_t				size_n_bits;
	struct s_binlist*	next;
	struct s_binlist*	prev;
};

struct s_fastbinlist {

	size_t					prevsize;
	size_t					size_n_bits;
	struct s_fastbinlist*	next;
};

/*
** Allocated chunk:
**
**	chunk ->		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of previous chunk, if freed (8 bytes)        |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             Size of chunk, in bytes (8 bytes)             |I|P|
**	mem ->			+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             User data                                         |
**					|                                                               |
**	nextchunk ->	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**					|             User data (8 bytes)                               |
**					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**
**	Note that the 8 first bytes of nextchunk are user data when it is allocated
*/
struct s_alloc_chunk {

	size_t			prevsize;
	size_t			size_n_bits;
};

/*
** Any chunk (cleaner code)
*/
struct s_any_chunk {

	size_t			prevsize;
	size_t			size_n_bits;
};

/*
** Arena header
**
** Arena first chunk: PREVINUSE always set
** Arena last chunk: ISTOPCHUNK always set
**		if is also the the topchunk of arenalist:
**			size is 0 and PREVINUSE is always set
**		else:
**			size is 16 + lost memory (can be 0 or 16)
*/
struct s_arena {

	struct s_arena*		prev;
};

//TODO: optimize size
struct s_malloc_struct {

	unsigned int			mmap_threshold; // TODO: the more you free, the more it grows
	struct s_arena			*tinyarenalist;
	struct s_any_chunk		*topchunk_tinyarena; // pointer on the last chunk of tinyarenalist
	struct s_arena			*smallarenalist;
	struct s_any_chunk		*topchunk_smallarena; // pointer on the last chunk of tinyarenalist

	// smallbin and tinybin are guaranteed to be coalesced
	// [0, 1]: smallunsorted
	struct s_binlist*		bin[NBINS * 2]; // ??? change array size one for each size of tiny arena + one for each n of small arena

	// 32 48 64 80 96 112 128 144 160 176 192 208 224 240 256 272
	// fastbin is a special list that store recently freed chunk.
	// Bits are not set in fastbin for faster alloc.
	struct s_fastbinlist*	fastbin[(FASTBIN_MAX >> 4) - 1];
};

/*
**	chunk_op.c
*/
inline unsigned int			get_bits(void *ptr);
inline void					add_bits(void *ptr, unsigned int bits);
inline void					rm_bits(void *ptr, unsigned int bits);
inline size_t				get_chunk_size(void *ptr);
inline void					set_freed_chunk_size(void *ptr, size_t sz);
inline void					set_alloc_chunk_size(void *ptr, size_t sz);
inline void					*ptr_offset(void *ptr, size_t offset);
inline struct s_any_chunk	*get_next_chunk(void* ptr);
inline size_t				chunk_size_from_user_size(size_t user_data);

void						*malloc(size_t size);
void						free(void *ptr);

struct s_binlist			*coalesce_tinychunk(struct s_any_chunk *chunk_ptr);
struct s_binlist			*coalesce_smallchunk(struct s_binlist *chunk_ptr);
void						unlink_chunk(struct s_binlist *chunk_ptr);
void						add_tinybin(struct s_binlist* chunk_ptr);


#endif
