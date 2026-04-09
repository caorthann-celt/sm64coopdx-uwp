#include <string.h>

void bzero(void* ptr, size_t size) {
    memset(ptr, 0, size);
}
