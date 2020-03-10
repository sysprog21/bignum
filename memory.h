#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *(*orig_malloc)(size_t) = malloc;
static void *(*orig_realloc)(void *, size_t) = realloc;
static void (*orig_free)(void *) = free;

/* TODO: implement custom memory allocator which fits arbitrary precision
 * operations
 */
static inline void *xmalloc(size_t size)
{
    void *p;
    if (!(p = (*orig_malloc)(size))) {
        fprintf(stderr, "Out of memory.\n");
        abort();
    }
    return p;
}

static inline void *xrealloc(void *ptr, size_t size)
{
    void *p;
    if (!(p = (*orig_realloc)(ptr, size)) && size != 0) {
        fprintf(stderr, "Out of memory.\n");
        abort();
    }
    return p;
}

static inline void xfree(void *ptr)
{
    (*orig_free)(ptr);
}

#define MALLOC(n) xmalloc(n)
#define REALLOC(p, n) xrealloc(p, n)
#define FREE(p) xfree(p)

#endif /* !_MEMORY_H_ */
