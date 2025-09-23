#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

// Include structs.h for type definitions
#include "structs.h"

// Memory allocation tracking structure
typedef struct memory_block {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    const char *function;
    struct memory_block *next;
} memory_block_t;

// Memory manager structure
typedef struct {
    memory_block_t *allocations;
    size_t total_allocated;
    size_t peak_allocated;
    size_t allocation_count;
    bool debug_enabled;
} memory_manager_t;

// Global memory manager
extern memory_manager_t g_memory_manager;

// Memory management macros
#define CR_MALLOC(size) cr_malloc_debug((size), __FILE__, __LINE__, __func__)
#define CR_CALLOC(count, size) cr_calloc_debug((count), (size), __FILE__, __LINE__, __func__)
#define CR_FREE(ptr) cr_free_debug((ptr), __FILE__, __LINE__, __func__)

// Core memory functions
void* cr_malloc_debug(size_t size, const char *file, int line, const char *function);
void* cr_calloc_debug(size_t count, size_t size, const char *file, int line, const char *function);
void cr_free_debug(void *ptr, const char *file, int line, const char *function);

// Memory manager functions
void memory_manager_init(void);
void memory_manager_cleanup(void);
void memory_manager_enable_debug(bool enable);
void memory_manager_print_stats(void);
void memory_manager_print_leaks(void);
bool memory_manager_check_integrity(void);

// Memory pool for frequent allocations
typedef struct memory_pool {
    void *pool;
    size_t block_size;
    size_t pool_size;
    size_t blocks_allocated;
    bool *allocation_map;
} memory_pool_t;

// Memory pools for common structures
extern memory_pool_t g_troop_pool;
extern memory_pool_t g_projectile_pool;
extern memory_pool_t g_spell_pool;
extern memory_pool_t g_building_pool;

// Pool management functions
memory_pool_t* create_memory_pool(size_t block_size, size_t num_blocks);
void* pool_alloc(memory_pool_t *pool);
bool pool_free(memory_pool_t *pool, void *ptr);
void destroy_memory_pool(memory_pool_t *pool);

// Pre-allocated pools
void init_game_memory_pools(void);
void cleanup_game_memory_pools(void);

// Stack trace file functions
void create_memory_trace_file(void);
void write_allocation_trace(const char *operation, void *ptr, size_t size, const char *file, int line, const char *function);
void write_memory_summary(void);

#ifdef __cplusplus
}
#endif

#endif