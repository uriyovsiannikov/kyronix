#pragma once

void klog_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#define log_info(fmt, ...) klog_printf("I: " fmt "\n", ##__VA_ARGS__)
#define log_warn(fmt, ...) klog_printf("W: " fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) klog_printf("E: " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) klog_printf("D: " fmt "\n", ##__VA_ARGS__)
