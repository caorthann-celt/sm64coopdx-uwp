#include <string.h>

// Tiny libc helper, grand for the bits of SM64 code that still call bzero
void bzero(void* ptr, size_t size) {
    memset(ptr, 0, size);
}
