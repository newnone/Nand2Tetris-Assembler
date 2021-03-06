// MIT License
// 
// Copyright (c) 2018 Oscar
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include "memcache.h"
#include <string.h>

memcache_t* n2t_memcache_alloc(uint32_t units, uint32_t unitsize) {
	memcache_t *o;

	if (units < 1 || unitsize < 1)
		return NULL;

	if ((o = malloc(sizeof(memcache_t))) == NULL)
		return NULL;
	
	o->head = calloc(units, unitsize);

	if (o->head == NULL) {
		free(o);
		return NULL;
	}

	o->unitsize = unitsize;
	o->next = 0;
	o->length = units;
	
	return o;
}

int n2t_memcache_extend(memcache_t *c, uint32_t n) {
	void *updated_head;

	if (n > 0) {
		updated_head = realloc(c->head, c->unitsize * (c->length + n));

		if (updated_head == NULL)
			return 1;

		c->head = updated_head;
		c->length += n;
	}

	return 0;
}

int64_t n2t_memcache_store(memcache_t *c, void const *source, uint32_t objsize) {
	if (source == NULL) {
		return -1;
	} else if (objsize > c->unitsize) {
		return -2;
	} else if (n2t_memcache_fetch(c, source, objsize) != NULL) {
		return -3;
	}
	
	if (MEMCACHE_FULL(c)) {
		n2t_memcache_extend(c, MEMCACHE_DEFAULT_EXTEND);
	}

	memcpy(
		c->head + MEMCACHE_OFFSET(c, c->next), source,
		MIN(c->unitsize, objsize)
	);
	// Set the remaining memory to `0', if any.
	memset(
		c->head + MEMCACHE_OFFSET(c, c->next) + MIN(c->unitsize, objsize), 0,
		c->unitsize - MIN(c->unitsize, objsize)
	);
	c->next++;

	return c->next - 1;
}

void* n2t_memcache_fetch(memcache_t const *c, void const *mould, uint32_t mouldsize) {
	uint32_t const cmpsize = MIN(c->unitsize, mouldsize);
	size_t i;

	if (mould == NULL)
		return NULL;
	if (mouldsize > c->unitsize)
		return NULL;

	for (i = 0; i < c->next; i++) {
		if (memcmp(c->head + MEMCACHE_OFFSET(c, i), mould, cmpsize) == 0) {
			return c->head + MEMCACHE_OFFSET(c, i);
		}
	}

	return NULL;
}

int64_t n2t_memcache_index_of(
	memcache_t const *c, void const *mould, uint32_t mouldsize
) {
	uint32_t const cmpsize = MIN(c->unitsize, mouldsize);
	uint32_t o;

	if (mouldsize > c->unitsize)
		return -2;
	else if (mould == NULL)
		return -3;

	for (o = 0; o < c->next; o++) {
		if (memcmp(c->head + MEMCACHE_OFFSET(c, o), mould, cmpsize) == 0)
			return o;
	}

	return -1;
}

void* n2t_memcache_index_fetch(memcache_t const *c, uint32_t index) {
	if (index >= c->next)
		return NULL;

	return c->head + MEMCACHE_OFFSET(c, index);
}

void n2t_memcache_free(memcache_t *c) {
	free(c->head);
	free(c);
}
