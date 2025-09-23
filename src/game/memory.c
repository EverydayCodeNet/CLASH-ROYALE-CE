#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <fileioc.h>

// Global memory manager instance
memory_manager_t g_memory_manager = {0};

// Pre-allocated memory pools
memory_pool_t g_troop_pool = {0};
memory_pool_t g_projectile_pool = {0};
memory_pool_t g_spell_pool = {0};
memory_pool_t g_building_pool = {0};

void memory_manager_init(void) {
    g_memory_manager.allocations = NULL;
    g_memory_manager.total_allocated = 0;
    g_memory_manager.peak_allocated = 0;
    g_memory_manager.allocation_count = 0;
    g_memory_manager.debug_enabled = true;
    
    create_memory_trace_file();
}

void memory_manager_cleanup(void) {
    if (g_memory_manager.debug_enabled) {
        memory_manager_print_leaks();
        write_memory_summary();
    }
    
    // Free all tracked allocations
    memory_block_t *current = g_memory_manager.allocations;
    while (current != NULL) {
        memory_block_t *next = current->next;
        free(current->ptr);
        free(current);
        current = next;
    }
    
    g_memory_manager.allocations = NULL;
    g_memory_manager.total_allocated = 0;
    g_memory_manager.allocation_count = 0;
}

void memory_manager_enable_debug(bool enable) {
    g_memory_manager.debug_enabled = enable;
}

void* cr_malloc_debug(size_t size, const char *file, int line, const char *function) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        return NULL;
    }
    
    if (g_memory_manager.debug_enabled) {
        // Track allocation
        memory_block_t *block = malloc(sizeof(memory_block_t));
        if (block != NULL) {
            block->ptr = ptr;
            block->size = size;
            block->file = file;
            block->line = line;
            block->function = function;
            block->next = g_memory_manager.allocations;
            g_memory_manager.allocations = block;
            
            g_memory_manager.total_allocated += size;
            g_memory_manager.allocation_count++;
            
            if (g_memory_manager.total_allocated > g_memory_manager.peak_allocated) {
                g_memory_manager.peak_allocated = g_memory_manager.total_allocated;
            }
            
            write_allocation_trace("MALLOC", ptr, size, file, line, function);
        }
    }
    
    return ptr;
}

void* cr_calloc_debug(size_t count, size_t size, const char *file, int line, const char *function) {
    void *ptr = calloc(count, size);
    if (ptr == NULL) {
        return NULL;
    }
    
    size_t total_size = count * size;
    
    if (g_memory_manager.debug_enabled) {
        // Track allocation
        memory_block_t *block = malloc(sizeof(memory_block_t));
        if (block != NULL) {
            block->ptr = ptr;
            block->size = total_size;
            block->file = file;
            block->line = line;
            block->function = function;
            block->next = g_memory_manager.allocations;
            g_memory_manager.allocations = block;
            
            g_memory_manager.total_allocated += total_size;
            g_memory_manager.allocation_count++;
            
            if (g_memory_manager.total_allocated > g_memory_manager.peak_allocated) {
                g_memory_manager.peak_allocated = g_memory_manager.total_allocated;
            }
            
            write_allocation_trace("CALLOC", ptr, total_size, file, line, function);
        }
    }
    
    return ptr;
}

void cr_free_debug(void *ptr, const char *file, int line, const char *function) {
    if (ptr == NULL) {
        return;
    }
    
    if (g_memory_manager.debug_enabled) {
        // Find and remove from tracking
        memory_block_t *current = g_memory_manager.allocations;
        memory_block_t *prev = NULL;
        
        while (current != NULL) {
            if (current->ptr == ptr) {
                // Remove from list
                if (prev == NULL) {
                    g_memory_manager.allocations = current->next;
                } else {
                    prev->next = current->next;
                }
                
                g_memory_manager.total_allocated -= current->size;
                g_memory_manager.allocation_count--;
                
                write_allocation_trace("FREE", ptr, current->size, file, line, function);
                
                free(current);
                break;
            }
            prev = current;
            current = current->next;
        }
        
        if (current == NULL) {
            // Double free or free of untracked pointer
            write_allocation_trace("ERROR_FREE", ptr, 0, file, line, function);
        }
    }
    
    free(ptr);
}

void memory_manager_print_stats(void) {
    if (!g_memory_manager.debug_enabled) return;
    
    // Print to trace file
    ti_var_t trace_file = ti_Open("CRMEMTRC", "a");
    if (trace_file) {
        char buffer[256];
        sprintf(buffer, "\n=== MEMORY STATISTICS ===\n");
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        sprintf(buffer, "Current allocated: %u bytes\n", (unsigned int)g_memory_manager.total_allocated);
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        sprintf(buffer, "Peak allocated: %u bytes\n", (unsigned int)g_memory_manager.peak_allocated);
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        sprintf(buffer, "Active allocations: %u\n", (unsigned int)g_memory_manager.allocation_count);
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        ti_Close(trace_file);
    }
}

void memory_manager_print_leaks(void) {
    if (!g_memory_manager.debug_enabled) return;
    
    ti_var_t trace_file = ti_Open("CRMEMTRC", "a");
    if (trace_file) {
        char buffer[256];
        sprintf(buffer, "\n=== MEMORY LEAKS ===\n");
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        memory_block_t *current = g_memory_manager.allocations;
        int leak_count = 0;
        
        while (current != NULL) {
            sprintf(buffer, "LEAK: %p (%u bytes) at %s:%d in %s\n", 
                   current->ptr, (unsigned int)current->size, 
                   current->file, current->line, current->function);
            ti_Write(buffer, strlen(buffer), 1, trace_file);
            leak_count++;
            current = current->next;
        }
        
        if (leak_count == 0) {
            sprintf(buffer, "No memory leaks detected!\n");
            ti_Write(buffer, strlen(buffer), 1, trace_file);
        }
        
        ti_Close(trace_file);
    }
}

bool memory_manager_check_integrity(void) {
    if (!g_memory_manager.debug_enabled) return true;
    
    size_t calculated_total = 0;
    size_t calculated_count = 0;
    
    memory_block_t *current = g_memory_manager.allocations;
    while (current != NULL) {
        calculated_total += current->size;
        calculated_count++;
        current = current->next;
    }
    
    return (calculated_total == g_memory_manager.total_allocated) && 
           (calculated_count == g_memory_manager.allocation_count);
}

// Memory pool implementation
memory_pool_t* create_memory_pool(size_t block_size, size_t num_blocks) {
    memory_pool_t *pool = CR_MALLOC(sizeof(memory_pool_t));
    if (pool == NULL) return NULL;
    
    pool->block_size = block_size;
    pool->pool_size = num_blocks;
    pool->blocks_allocated = 0;
    
    // Allocate the pool memory
    pool->pool = CR_MALLOC(block_size * num_blocks);
    if (pool->pool == NULL) {
        CR_FREE(pool);
        return NULL;
    }
    
    // Allocate allocation map
    pool->allocation_map = CR_CALLOC(num_blocks, sizeof(bool));
    if (pool->allocation_map == NULL) {
        CR_FREE(pool->pool);
        CR_FREE(pool);
        return NULL;
    }
    
    return pool;
}

void* pool_alloc(memory_pool_t *pool) {
    if (pool == NULL || pool->blocks_allocated >= pool->pool_size) {
        return NULL;
    }
    
    // Find first free block
    for (size_t i = 0; i < pool->pool_size; i++) {
        if (!pool->allocation_map[i]) {
            pool->allocation_map[i] = true;
            pool->blocks_allocated++;
            return (char*)pool->pool + (i * pool->block_size);
        }
    }
    
    return NULL;
}

bool pool_free(memory_pool_t *pool, void *ptr) {
    if (pool == NULL || ptr == NULL) return false;
    
    // Calculate block index
    char *block_ptr = (char*)ptr;
    char *pool_start = (char*)pool->pool;
    
    if (block_ptr < pool_start || 
        block_ptr >= pool_start + (pool->pool_size * pool->block_size)) {
        return false; // Not from this pool
    }
    
    size_t index = (block_ptr - pool_start) / pool->block_size;
    
    if (index < pool->pool_size && pool->allocation_map[index]) {
        pool->allocation_map[index] = false;
        pool->blocks_allocated--;
        return true;
    }
    
    return false;
}

void destroy_memory_pool(memory_pool_t *pool) {
    if (pool == NULL) return;
    
    CR_FREE(pool->allocation_map);
    CR_FREE(pool->pool);
    CR_FREE(pool);
}

void init_game_memory_pools(void) {
    // Create pools for frequent allocations
    memory_pool_t *troop_pool = create_memory_pool(sizeof(troop_t), 32);
    memory_pool_t *projectile_pool = create_memory_pool(sizeof(projectile_t), 64);
    memory_pool_t *spell_pool = create_memory_pool(sizeof(spell_t), 16);
    memory_pool_t *building_pool = create_memory_pool(sizeof(building_t), 16);
    
    if (troop_pool) g_troop_pool = *troop_pool;
    if (projectile_pool) g_projectile_pool = *projectile_pool;
    if (spell_pool) g_spell_pool = *spell_pool;
    if (building_pool) g_building_pool = *building_pool;
}

void cleanup_game_memory_pools(void) {
    destroy_memory_pool(&g_troop_pool);
    destroy_memory_pool(&g_projectile_pool);
    destroy_memory_pool(&g_spell_pool);
    destroy_memory_pool(&g_building_pool);
}

void create_memory_trace_file(void) {
    ti_var_t trace_file = ti_Open("CRMEMTRC", "w");
    if (trace_file) {
        char header[] = "=== CLASH ROYALE MEMORY TRACE ===\n";
        ti_Write(header, strlen(header), 1, trace_file);
        ti_Close(trace_file);
    }
}

void write_allocation_trace(const char *operation, void *ptr, size_t size, const char *file, int line, const char *function) {
    if (!g_memory_manager.debug_enabled) return;
    
    ti_var_t trace_file = ti_Open("CRMEMTRC", "a");
    if (trace_file) {
        char buffer[256];
        sprintf(buffer, "%s: %p (%u bytes) at %s:%d in %s\n", 
               operation, ptr, (unsigned int)size, file, line, function);
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        ti_Close(trace_file);
    }
}

void write_memory_summary(void) {
    if (!g_memory_manager.debug_enabled) return;
    
    ti_var_t trace_file = ti_Open("CRMEMTRC", "a");
    if (trace_file) {
        char buffer[256];
        sprintf(buffer, "\n=== FINAL MEMORY SUMMARY ===\n");
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        sprintf(buffer, "Peak memory usage: %u bytes\n", (unsigned int)g_memory_manager.peak_allocated);
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        sprintf(buffer, "Final allocated memory: %u bytes\n", (unsigned int)g_memory_manager.total_allocated);
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        sprintf(buffer, "Active allocations at exit: %u\n", (unsigned int)g_memory_manager.allocation_count);
        ti_Write(buffer, strlen(buffer), 1, trace_file);
        
        ti_Close(trace_file);
    }
}