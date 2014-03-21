#include <cstdio>
#include <thread>
#include <atomic>

#undef DEBUG_MALLOC

static std::atomic_size_t mem_usage(0);
static std::atomic_size_t peak_mem_usage(0);
static std::atomic_size_t total_mem_usage(0);
static std::atomic_size_t allocs(0);
static std::atomic_size_t peak_allocs(0);
static std::atomic_size_t total_allocs(0);
static std::atomic_size_t mem_limit(100 * 1024*1024);

#ifdef WRAP_MALLOC

extern "C" {

void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void* ptr);
void* __real_realloc(void* oldptr, size_t size);


void* __wrap_malloc(size_t size)
{
    if (mem_usage + size+16 > mem_limit) {
        printf("\ntrying to allocate %lld bytes when usage already %lld bytes", size+16, mem_usage.load());
        printf("\nmemory limit hit!\n\n");
        return nullptr;
    }

    mem_usage += size+16;
    total_mem_usage += size+16;
    
    // if (peak_mem_usage < mem_usage) {
    //     peak_mem_usage = mem_usage;
    // }

    allocs++;
    // if (peak_allocs < allocs) peak_allocs = allocs;
    total_allocs++;

    void* p = __real_malloc(size+16);
    *(size_t*)p = size;

#ifdef DEBUG_MALLOC
    printf("MALLOC: %p : %lld malloc\n", (char*)p+16, size);
#endif

    return (char*)p+16;
}

void* __wrap_calloc(size_t num, size_t elsize)
{
    size_t size = num*elsize;

    if (mem_usage + size+16 > mem_limit) {
        printf("\ntrying to allocate %lld bytes when usage already %lld bytes", size+16, mem_usage.load());
        printf("\nmemory limit hit!\n\n");
        return nullptr;
    }

    mem_usage += size+16;
    total_mem_usage += size+16;

    // if (peak_mem_usage < mem_usage) {
    //     peak_mem_usage = mem_usage;
    // }

    allocs++;
    // if (peak_allocs < allocs) peak_allocs = allocs;
    total_allocs++;

    void* p = __real_calloc(size+16, 1);
    *(size_t*)p = size;

#ifdef DEBUG_MALLOC
    printf("MALLOC: %p : %lld calloc\n", (char*)p+16, size);
#endif

    return (char*)p+16;
}

void* __wrap_realloc(void* oldptr, size_t size)
{
    if (oldptr == nullptr) return malloc(size);
    
    void* p = (char*)oldptr-16;

    void* new_p = __real_realloc(p, size);
    if (new_p == nullptr) {
#ifdef DEBUG_MALLOC
        printf("MALLOC: %p : %lld realloc\n", (char*)oldptr, size);
#endif
        return nullptr;
    }

    size_t old_size = *(size_t*)new_p;
    if (old_size > size) {
        mem_usage -= old_size;
        mem_usage += size;
        *(size_t*)new_p = size;
    }

#ifdef DEBUG_MALLOC
    printf("MALLOC: %p : %lld realloc %lld\n", (char*)oldptr, size, old_size);
#endif

    return (char*)new_p + 16;
}

void __wrap_free(void* ptr)
{
    if (ptr == nullptr) {
#ifdef DEBUG_MALLOC
        printf("MALLOC: %p free\n", ptr);
#endif
        return;
    }

    void* p = (char*)ptr-16;
    size_t s = *(size_t*)p;
    mem_usage -= s+16;
    allocs--;
#ifdef DEBUG_MALLOC
    printf("MALLOC: %p : %lld free\n", (char*)p+16, s);
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

#endif // WRAP_MALLOC


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

size_t get_total_mem_usage ()
{
    return total_mem_usage;
}

size_t get_mem_allocs ()
{
    return allocs;
}

size_t get_peak_mem_allocs ()
{
    return peak_allocs;
}

size_t get_total_mem_allocs ()
{
    return total_allocs;
}
