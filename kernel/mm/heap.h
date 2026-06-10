#pragma once
#include <stddef.h>
#include <stdint.h>

#define HEAP_START 0xffff910000000000ULL
#define HEAP_MAX 0xffff920000000000ULL

void heap_init(void);

void* kmalloc(uint64_t size);
void* kcalloc(uint64_t nmemb, uint64_t size);
void* krealloc(void* ptr, uint64_t new_size);
void kfree(void* ptr);

void heap_stats(void);
