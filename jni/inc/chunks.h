#ifndef CHUNKS_H
#define CHUNKS_H

#ifdef __cplusplus
extern "C" {
#endif

/*  chunks_t maintains lists of used and ready chunks of the same size.
    releasing chunk_t with ref_counter == 1 (before release) does not
    call mem_free but keeps the chunk for later reuse. Thread safe.
    A CHUNK IS NOT SUPPOSED TO BE ACCESSED BY MORE THAN ONE THREAD CONCURRENTLY.
    THUS NO SYNCHRONIZATION PER CHUNK IS PROVIDED. THE WHOLE IDEA OF REF COUNTING
    OF CHUNKS IS TO SAFELY PASS OWNERSHIP OF A CHUNK FROM ONE THREAD TO ANOTHER
    VIA A QUEUE.
*/

typedef void* chunks_t;

typedef struct chunk_s {
    int bytes; /* including sizeof(chunk_s) */
    volatile int ref_count;
    chunks_t chunks;
    struct chunk_s* next;
    struct chunk_s* back;
} chunk_t;

chunks_t chunks_create();

/* if no chunk of specified number of bytes is available all ready chunks
   with different size are mem_free-ed to the system memory and new chunk
   is mem_alloc-ed. The reference count is 1.
   Chunk memory is NOT zero-ed out. bytes parameter should be inclusive of
   chunk_t size. Useful client memory starts after chunk.
   May return null. */
chunk_t* chunks_get(chunks_t cs, int bytes);

void chunk_addref(chunk_t* c);
void chunk_release(chunk_t* c);

int chunks_ready(chunks_t cs); /* number of chunks ready for reuse */
int chunks_used(chunks_t cs);  /* number of chunks with ref counter > 0 */

void chunks_gc(chunks_t cs); /* garbage collect mem_free for all ready chunks */

void chunks_destroy(chunks_t cs);

#ifdef __cplusplus
}
#endif

#endif /* CHUNKS_H */
