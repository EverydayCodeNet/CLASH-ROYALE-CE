#include "memory_simple.h"
#include <stdio.h>
#include <string.h>

// Safe allocation wrappers with null checks
void* safe_malloc(size_t size) {
    if (size == 0) return NULL;

    void *ptr = malloc(size);
    if (ptr == NULL) {
        // Handle allocation failure gracefully
        return NULL;
    }
    return ptr;
}

void* safe_calloc(size_t count, size_t size) {
    if (count == 0 || size == 0) return NULL;

    void *ptr = calloc(count, size);
    if (ptr == NULL) {
        // Handle allocation failure gracefully
        return NULL;
    }
    return ptr;
}

void* safe_realloc(void *ptr, size_t size) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        // Original pointer is still valid on failure
        return NULL;
    }
    return new_ptr;
}

void safe_free(void **ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

#ifdef MEMORY_DEBUG
// Simple memory statistics implementation
simple_memory_stats_t g_memory_stats = {0, 0, false};

void memory_stats_init(void) {
    g_memory_stats.total_allocated = 0;
    g_memory_stats.allocation_count = 0;
    g_memory_stats.debug_enabled = true;
}

void memory_stats_print(void) {
    if (!g_memory_stats.debug_enabled) return;

    printf("Memory Stats:\n");
    printf("  Total allocated: %zu bytes\n", g_memory_stats.total_allocated);
    printf("  Active allocations: %zu\n", g_memory_stats.allocation_count);
}

void* debug_malloc(size_t size, const char *file, int line) {
    void *ptr = malloc(size);
    if (ptr && g_memory_stats.debug_enabled) {
        g_memory_stats.total_allocated += size;
        g_memory_stats.allocation_count++;
        printf("MALLOC: %p (%zu bytes) at %s:%d\n", ptr, size, file, line);
    }
    return ptr;
}

void* debug_calloc(size_t count, size_t size, const char *file, int line) {
    void *ptr = calloc(count, size);
    if (ptr && g_memory_stats.debug_enabled) {
        size_t total_size = count * size;
        g_memory_stats.total_allocated += total_size;
        g_memory_stats.allocation_count++;
        printf("CALLOC: %p (%zu bytes) at %s:%d\n", ptr, total_size, file, line);
    }
    return ptr;
}

void debug_free(void *ptr, const char *file, int line) {
    if (ptr && g_memory_stats.debug_enabled) {
        g_memory_stats.allocation_count--;
        printf("FREE: %p at %s:%d\n", ptr, file, line);
    }
    free(ptr);
}
#endif