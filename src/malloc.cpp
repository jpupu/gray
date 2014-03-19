#include <cstdio>
#include <thread>
#include <mutex>

#undef DEBUG_MALLOC

extern "C" {

void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void* ptr);
void* __real_realloc(void* oldptr, size_t size);

static std::mutex mtx;
static size_t mem_usage;
static size_t peak_mem_usage;
static size_t mem_limit = 100 * 1024*1024;

void* __wrap_malloc(size_t size)
{
    std::lock_guard<std::mutex> lck (mtx);

    if (mem_usage + size+16 > mem_limit) {
        printf("\ntrying to allocate %lld bytes when usage already %lld bytes", size+16, mem_usage);
        printf("\nmemory limit hit!\n\n");
        return nullptr;
    }

    mem_usage += size+16;

    if (peak_mem_usage < mem_usage) {
        peak_mem_usage = mem_usage;
    }

    void* p = __real_malloc(size+16);
    *(size_t*)p = size;

#ifdef DEBUG_MALLOC
    printf("alloc %lld bytes at %p, usage %lld\n", size, (char*)p+16, mem_usage);
#endif

    return (char*)p+16;
}

void* __wrap_calloc(size_t num, size_t elsize)
{
    std::lock_guard<std::mutex> lck (mtx);
    size_t size = num*elsize;

    if (mem_usage + size+16 > mem_limit) {
        printf("\ntrying to allocate %lld bytes when usage already %lld bytes", size+16, mem_usage);
        printf("\nmemory limit hit!\n\n");
        return nullptr;
    }

    mem_usage += size+16;

    if (peak_mem_usage < mem_usage) {
        peak_mem_usage = mem_usage;
    }

    void* p = __real_calloc(size+16, 1);
    *(size_t*)p = size;

#ifdef DEBUG_MALLOC
    printf("calloc %lld bytes at %p, usage %lld\n", size, (char*)p+16, mem_usage);
#endif

    return (char*)p+16;
}

void* __wrap_realloc(void* oldptr, size_t size)
{
    if (oldptr == nullptr) return malloc(size);
    
    std::lock_guard<std::mutex> lck (mtx);
    void* p = (char*)oldptr-16;

#ifdef DEBUG_MALLOC
    printf("realloc %lld bytes at %p, usage %lld\n", size, (char*)oldptr, mem_usage);
#endif

    void* new_p = __real_realloc(p, size);
    if (new_p == nullptr) {
        return nullptr;
    }

    size_t old_size = *(size_t*)new_p;
    if (old_size > size) {
        mem_usage -= old_size;
        mem_usage += size;
        *(size_t*)new_p = size;
    }

    return (char*)new_p + 16;
}

void __wrap_free(void* ptr)
{
    if (ptr == nullptr) return;

    std::lock_guard<std::mutex> lck (mtx);
    void* p = (char*)ptr-16;
    size_t s = *(size_t*)p;
    mem_usage -= s;
#ifdef DEBUG_MALLOC
    printf("free %lld bytes at %p, usage %lld\n", s, (char*)p+16, mem_usage);
#endif
    __real_free(p);
}

}

void* operator new (size_t size)
{
    void* p = malloc(size);
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}
void* operator new[ ] (size_t size)
{
    void* p = malloc(size);
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}
void operator delete (void* pointerToDelete) noexcept
{
    free(pointerToDelete);
}
void operator delete[ ] (void* arrayToDelete) noexcept
{
    free(arrayToDelete);
}

void set_mem_limit (size_t limit)
{
    mem_limit = limit;
}

size_t get_mem_limit ()
{
    return mem_limit;
}

size_t get_mem_usage ()
{
    return mem_usage;
}

size_t get_peak_mem_usage ()
{
    return peak_mem_usage;
}
