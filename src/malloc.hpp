#ifndef _MALLOC_HPP_
#define _MALLOC_HPP_

void set_mem_limit (size_t limit);
size_t get_mem_limit ();
size_t get_mem_usage ();
size_t get_peak_mem_usage ();

#endif // _MALLOC_HPP_