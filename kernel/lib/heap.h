#pragma once

#include <stddef.h>
#include <stdint.h>

void heap_init(uint64_t start_virt);
void *kmalloc(size_t size);
void kfree(void *ptr);
void heap_print(void);
