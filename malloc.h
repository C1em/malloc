/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/08/12 17:27:04 by coremart          #+#    #+#             */
/*   Updated: 2020/08/12 19:11:03 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

# define TINY_TRESHOLD		64
# define SMALL_TRESHOLD		256

# define NEXT_PW2(x)		1 << (32 - __builtin_clz(x - 1))

# define TINY_ARENA			NEXT_PW2(TINY_TRESHOLD * 100)
# define SMALL_ARENA		NEXT_PW2(SMALL_TRESHOLD * 100)

# define ARENA_SIZE			TINY_ARENA + SMALL_ARENA

struct u_size_n_previnuse
{
	size_t			size : sizeof(size_t) - 3;
	unsigned int	previnuse : 3;
};

struct s_binlist
{
	size_t						prev_size;
	struct u_size_n_previnuse	size_n_previnuse;
	struct s_binlist*			next;
	struct s_binlist*			prev;
};

struct s_alloc_chunk
{
	size_t						prev_size;
	struct u_size_n_previnuse	size_n_previnuse;
};

struct s_malloc_struct
{
	struct s_binlist*	bin_array[10]; // ??? change array size
	void				*arena;
};

struct s_malloc_struct malloc_struct;

void		*malloc(size_t size);

#endif
