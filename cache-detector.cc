#include <inttypes.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

enum { BUFSIZE = 32*1024*1024 };
enum { CACHE_LINE_SIZE = 64 };
enum { NITERS = 100 };

static volatile uint32_t buf[BUFSIZE/4] = {0};

static inline uint64_t rdtsc(void)
{
#if defined(__i386__)
int64_t ret;
__asm__ volatile("rdtsc" : "=A"(ret));
return ret;
#elif defined(__x86_64__) || defined(__amd64__)
uint64_t low, high;
__asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
return (high << 32) | low;
#elif defined(__aarch64__)
// System timer of ARMv8 runs at a different frequency than the CPU's.
// The frequency is fixed, typically in the range 1-50MHz.  It can be
// read at CNTFRQ special register.  We assume the OS has set up
// the virtual timer properly.
int64_t virtual_timer_value;
asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
return virtual_timer_value;
#elif defined(__ARM_ARCH)
#if (__ARM_ARCH >= 6)  // V6 is the earliest arch that has a standard cyclecount
//userspace has no permission set perf monitor counter,so need kernel module enable(kernel-module)
uint32_t pmccntr;
uint32_t pmuseren;
uint32_t pmcntenset;
// Read the user mode perf monitor counter access permissions.
asm volatile("mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren));
if (pmuseren & 1) {  // Allows reading perfmon counters for user mode code.
    asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset));
    if (pmcntenset & 0x80000000ul) {  // Is it counting?
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));
        // The counter is set up to count every 64th cycle
        return static_cast<int64_t>(pmccntr) * 64;  // Should optimize to << 6
    }
}
#endif
#endif
return 0;
}

int real_priority()
{
    sched_param param;
    param.sched_priority = 1;
    // Set the priority; others are unchanged.
    printf("Log: Changing priority to SCHED_FIFO %d\n",
           param.sched_priority);
    if (sched_setscheduler(0, SCHED_FIFO, &param)) {
        printf("Process Error: sched_setscheduler "
               "failed - error %d\n",
               errno);
    }

    return 0;
}

int bind_to_cpu(int target_cpu)
{
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(target_cpu, &cpu_mask);
    if (sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask) < 0)
        return errno;

    return 0;
}

int
main(int argc, char **argv)
{
    int probe_size = 8;
    std::cout << "# (cycles per memory op) * 10\n";
    std::cout << "# bytes of memory accessed in random order\n";
    bind_to_cpu(1);
    real_priority();
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
