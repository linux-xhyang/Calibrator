#include <inttypes.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

enum { BUFSIZE = 32*1024*1024 };
enum { CACHE_LINE_SIZE = 64 };
enum { NITERS = 100 };

static volatile uint32_t buf[BUFSIZE/4] = {0};

static inline uint64_t rdtsc(void)
{
    uint64_t result;
    asm volatile("rdtsc" : "=A" (result));
    return result;
}

int
main(int argc, char **argv)
{
    int probe_size = 8;

    std::cout << "# (cycles per memory op) * 10\n";
    std::cout << "# bytes of memory accessed in random order\n";
    while (probe_size < BUFSIZE/CACHE_LINE_SIZE) {
	uint64_t time = 0;
	uint64_t count = 0;
	for (int x = 0; x < 16; ++x) {
	    for (int i = 0; i < probe_size; ++i)
		buf[i * CACHE_LINE_SIZE/4] = 0xffffffff;
	    int last_idx = 0;
	    for (int i = 1; i < probe_size; ++i) {
		int rnd_idx = random() % probe_size;
		buf[last_idx * CACHE_LINE_SIZE/4] = 123;
		while (buf[rnd_idx * CACHE_LINE_SIZE/4] != 0xffffffff) {
		    rnd_idx = (rnd_idx + 1);
		    if (rnd_idx > probe_size)
			rnd_idx = 0;
		}
		buf[last_idx * CACHE_LINE_SIZE/4] = rnd_idx;
		last_idx = rnd_idx;
	    }
	    buf[last_idx * CACHE_LINE_SIZE/4] = 0;

	    uint64_t start = rdtsc();
	    int niters = 1000000 / probe_size;
	    if (niters < 2)
		niters = 2;
	    for (int i = 0; i < niters; ++i) {
		++count;
		for (int idx = buf[0]; idx != 0; idx = buf[idx * CACHE_LINE_SIZE/4])
		    ++count;
	    }
	    time += rdtsc() - start;
	}

	std::cout.width(5);
	std::cout << (time / (count/10)) << " ";
	std::cout.width(8);
	std::cout << (probe_size * CACHE_LINE_SIZE) << "\n";
	if ((probe_size & (probe_size - 1)) == 0)
	    probe_size = probe_size * 3 / 2;
	else
	    probe_size = 2 * (probe_size & (probe_size - 1));
    }
    return 0;
}
