/* Bagian Hisyam - eksperimen malloc/free, monitoring, dan pengiriman record ke logger. */
#include "malloc_module.h"
#include "memory_monitor.h"
#include "logging.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void run_malloc_experiment(void)
{
    printf("You picked malloc/free.\n");

    MemoryMetrics before;
    read_memory_metrics(&before);

    printf("VmSize Before: %lu KB | VmRssBefore: %lu KB\n", before.vm_size_kb, before.vm_rss_kb);

    long N;
    do
    {
        printf("How many integers to allocate: ");
        scanf("%ld", &N);

        if (N <= 0)
        {
            printf("Input must be a positive integer.\n");
            continue;
        }
        else
            break;
    } while (1);

    long *p = malloc((size_t)N * sizeof(long));

    if (p == NULL)
    {
        perror("malloc failed.");
        return;
    }

    printf("Starting memory address: %p \n", (void *)p);

    long num_of_values = (N >= 10 ? 10 : N);
    printf("Printing %ld values in reverse order:\n", num_of_values);

    // assign with value from 1 to N
    for (long i = 0; i < N; i++)
    {
        p[i] = i + 1;
    }

    for (long i = 0; i < num_of_values; i++)
    {
        printf("%ld\n", p[N - i - 1]);
    }

    MemoryMetrics after;
    read_memory_metrics(&after);

    printf("VmSize After: %lu KB | VmRss After: %lu KB\n", after.vm_size_kb, after.vm_rss_kb);

    char record[1024];
    int used = snprintf(record, sizeof(record),
                        "How many integers to allocate: %ld\n"
                        "Starting memory address: %p\n"
                        "VmSize Before: %lu KB | VmRSS Before: %lu KB\n"
                        "Printing %ld values in reverse order:\n",
                        N, (void *)p, before.vm_size_kb, before.vm_rss_kb,
                        num_of_values);

    for (long i = 0; i < num_of_values; i++)
    {
        used += snprintf(record + used, sizeof(record) - (size_t)used, "%ld\n", p[N - i - 1]);
    }

    snprintf(record + used, sizeof(record) - (size_t)used,
             "VmSize After: %lu KB | VmRSS After: %lu KB",
             after.vm_size_kb, after.vm_rss_kb);

    write_logs("malloc", record);

    free(p);
    return;
}
