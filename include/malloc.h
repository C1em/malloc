/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:04 by coremart          #+#    #+#             */
/*   Updated: 2021/08/12 15:47:38 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>
# include <stdbool.h>
# include <pthread.h>

# define HEADER_SIZE			sizeof(struct s_any_chunk)

# define TINY_MIN				32
# define TINY_THRESHOLD			512
# define SMALL_THRESHOLD		10240
# define FASTBIN_MAX			272

# define PAGE_SZ				sysconf(_SC_PAGESIZE)
# define NEXT_8MULT(x)			((7 + x) & ~7)
# define NEXT_PAGEALIGN(x)		((PAGE_SZ - 1 + x) & ~(PAGE_SZ - 1))

# define TINY_ARENA_SZ			NEXT_PAGEALIGN(TINY_THRESHOLD * 100)
# define SMALL_ARENA_SZ			NEXT_PAGEALIGN(SMALL_THRESHOLD * 100)

# define PREVINUSE				0x1
# define ISTOPCHUNK				0x2 // the top chunk of an arena
# define BITS					0x7
# define CHUNK_SIZE				~0x7UL

# define NB_TINYBINS			(TINY_THRESHOLD / 32)
# define NB_SMALLBINS			(((SMALL_THRESHOLD / 10) - TINY_THRESHOLD) / 64 \
+ (SMALL_THRESHOLD / 10) / 128 \
+ (SMALL_THRESHOLD / 5) / 256 \
+ (SMALL_THRESHOLD / 5) / 512 \
+ (SMALL_THRESHOLD / 5) / 1024 \
+ (SMALL_THRESHOLD / 5) / 2048 \
+ 1)
// (10240 / 10) - 512) / 64 + (10240 / 10) / 128 + (10240 / 5) / 256 + (10240 / 5) / 512 + (10240 / 5) / 1024 + (10240 / 5) / 2048 + 1

# define NBINS					NB_TINYBINS + NB_SMALLBINS + NB_BIGFREED


extern pthread_mutex_t mutex;

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

/* *
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

struct s_malloc_struct {

	// smallbin and tinybin are guaranteed to be coalesced
	// [0..15]	smallbins < 1024: [0,1] is 512 to 575, [2,3] is 576 to 639 ..., [14,15] is 960 to 1023
	// [16..31]	smallbins < 2048: [16,17] is 1024 to 1151, [18, 19] is 1152 to 2303 ..., [30,31] is 1920 to 2047
	// [32..47]	smallbins < 4096: [32,33] is 2048 to 2303, [34,35] is 2304 to 2559 ...,[46,47] is 3840 to 4095
	// [48..55]	smallbins < 6144: [48,49] is 4096 to 4607, [50,51] is 4608 to 5119 ..., [54,55] is 5632 to 6143
	// [56..59]	smallbins < 8192: [56,57] is 6144 to 7167, [58,59] is 7168 to 8191
	// [60,61]	smallbins < 10240: [60,61] is 8192 to 10239
	// [62,63]	smallbins >= 10240
	struct s_binlist*		smallbin[NB_SMALLBINS * 2];

	// [0..31] tinybins: [0,1] is 32 to 63, [2,3] is 64 to 95 ..., [30,31] is 512 and more
	struct s_binlist*		tinybin[NB_TINYBINS * 2];


	// fastbin is a special list that store recently freed chunk.
	// Bits are not set in fastbin for faster alloc.
	// 32 48 64 80 96 112 128 144 160 176 192 208 224 240 256 272
	struct s_fastbinlist*	fastbin[(FASTBIN_MAX >> 4) - 1];

	struct s_arena			*tinyarenalist;
	struct s_any_chunk		*topchunk_tinyarena; // pointer on the last chunk of tinyarenalist
	struct s_arena			*smallarenalist;
	struct s_any_chunk		*topchunk_smallarena; // pointer on the last chunk of tinyarenalist
	struct s_arena			*largearenalist;
};

/**
 * chunk_op.c
 */
unsigned int			get_bits(void *ptr);
void					add_bits(void *ptr, unsigned int bits);
void					rm_bits(void *ptr, unsigned int bits);
size_t					get_chunk_size(void *ptr);
void					set_freed_chunk_size(void *ptr, size_t sz);
void					set_alloc_chunk_size(void *ptr, size_t sz);
void					*ptr_offset(void *ptr, long offset);
struct s_any_chunk		*get_next_chunk(void* ptr);
struct s_any_chunk		*get_prev_chunk(void* ptr);
struct s_any_chunk		*split_tinychunk_for_size(struct s_any_chunk *chunk_ptr, size_t sz);
size_t					chunk_size_from_user_size(size_t user_data);
bool					is_valid_chunk(struct s_alloc_chunk *chunk);


void					*malloc(size_t size);
void					free(void *ptr);
void					*realloc(void *ptr, size_t size);
void					*calloc(size_t count, size_t size);
void					*reallocf(void *ptr, size_t size);
size_t					malloc_size(const void* ptr);
size_t					malloc_good_size(size_t size);
void					show_alloc_mem(void);

bool					is_valid_chunk(struct s_alloc_chunk *chunk);

/**
 * bin_utils.c
 */
void					add_fastbin(struct s_alloc_chunk *chunk);
void					add_big_tinybin(struct s_binlist *chunk);
void					add_big_smallbin(struct s_binlist *chunk);
int						get_smallbin_index(size_t sz);
void					add_smallbin(struct s_binlist *chunk);
void					add_tinybin(struct s_binlist *chunk);
void					unlink_chunk(struct s_binlist *chunk);
struct s_alloc_chunk	*check_big_smallbin(size_t size);
struct s_alloc_chunk	*check_smallbin(size_t size);
struct s_alloc_chunk	*check_fastbin(size_t size);
struct s_alloc_chunk	*check_tinybin(size_t size);
struct s_alloc_chunk	*check_big_tinybin(size_t size);
struct s_alloc_chunk	*coalesce_fastbin(size_t size);

/**
 * coalesce_chunk.c
 */
struct s_binlist		*coalesce_tinychunk(struct s_any_chunk *chunk_ptr);
struct s_binlist		*coalesce_smallchunk(struct s_any_chunk *chunk_ptr);

void					unlink_chunk(struct s_binlist *chunk_ptr);
void					add_tinybin(struct s_binlist* chunk_ptr);
int						get_smallbin_index(size_t sz);
void					add_smallbin(struct s_binlist *chunk);

/**
 * print_utils.c
 */
void					print_size(size_t sz);
void					print_addr(void *addr);
void					print_alloc_chunk(struct s_any_chunk *chunkptr);
void					print_alloc_chunk(struct s_any_chunk *chunkptr);
size_t					print_tinyarenas(struct s_arena *tinyarenalist);
size_t					print_smallarenas(struct s_arena *smallarenalist);
size_t					print_largearenas(struct s_arena *largearenalist);

/**
 * arena_utils.c
 */
bool					unlink_largearena(struct s_arena* arena);
bool					is_in_arena(struct s_alloc_chunk *chunk);
struct s_alloc_chunk	*new_smallarena(size_t size);
struct s_alloc_chunk	*new_tinyarena(size_t size);


#endif
