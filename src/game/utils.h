#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>

typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    unsigned int MAX;
    int current;
} health_t;


#ifdef __cplusplus
}
#endif

#endif