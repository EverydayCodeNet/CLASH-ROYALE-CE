#ifndef MEMORY_SIMPLE_H
#define MEMORY_SIMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

// Simple memory management using standard C functions
// No pools, no complex tracking, just clean wrappers

// Memory allocation macros - direct to standard C functions
#define CR_MALLOC(size) malloc(size)
#define CR_CALLOC(count, size) calloc(count, size)
#define CR_REALLOC(ptr, size) realloc(ptr, size)
#define CR_FREE(ptr) do { if (ptr) { free(ptr); (ptr) = NULL; } } while(0)

// Safe allocation wrappers with null checks
void* safe_malloc(size_t size);
void* safe_calloc(size_t count, size_t size);
void* safe_realloc(void *ptr, size_t size);
void safe_free(void **ptr);

// Optional: Simple memory statistics (disabled by default)
#ifdef MEMORY_DEBUG
typedef struct {
    size_t total_allocated;
    size_t allocation_count;
    bool debug_enabled;
} simple_memory_stats_t;

extern simple_memory_stats_t g_memory_stats;

void memory_stats_init(void);
void memory_stats_print(void);
void* debug_malloc(size_t size, const char *file, int line);
void* debug_calloc(size_t count, size_t size, const char *file, int line);
void debug_free(void *ptr, const char *file, int line);

// Debug macros (only active if MEMORY_DEBUG is defined)
#undef CR_MALLOC
#undef CR_CALLOC
#undef CR_FREE
#define CR_MALLOC(size) debug_malloc(size, __FILE__, __LINE__)
#define CR_CALLOC(count, size) debug_calloc(count, size, __FILE__, __LINE__)
#define CR_FREE(ptr) debug_free(ptr, __FILE__, __LINE__)

#endif

#ifdef __cplusplus
}
#endif

#endif