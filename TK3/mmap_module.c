/* Bagian Fiqhi - eksperimen mmap/munmap, monitoring, dan pengiriman record ke logger. */
#include "mmap_module.h"
#include "memory_monitor.h"
#include "logging.h"
#include <stdio.h>
#include <sys/mman.h>

void run_mmap_experiment(void) {
    /* TODO: Input N, mmap/munmap, sampling nilai, monitoring, logging. */
    printf("You picked mmap/umunmap.\n");

    MemoryMetrics before, after;
    if (read_memory_metrics(&before) != 0) {
        fprintf(stderr, "Failed to read memory metrics\n");
        return;
    }

    printf("VmSize Before: %lu KB | VmRSS Before: %lu KB\n", before.vm_size_kb, before.vm_rss_kb);

    int n_allocation;
    printf("How many integers to allocate: ");
    if(scanf("%d", &n_allocation) != 1 || n_allocation <= 0) {
        fprintf(stderr, "Error: Please enter a positive integer");
        return;
    }

    long *ptr = mmap(NULL, n_allocation * sizeof(long), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return;
    }

    printf("Starting memory address: %p\n", (void *)ptr);
    for (long i=n_allocation - 1; i>=0; i--) {
        ptr[i] = i + 1;
    }
    
    printf("Printing the first 10 values in reverse order: \n");
    
    int count = n_allocation < 10 ? n_allocation : 10;

    for (long i=0; i<count; i++) {
        printf("%ld\n", ptr[n_allocation - i - 1]);
    }

    if (read_memory_metrics(&after) != 0) {
        fprintf(stderr, "Failed to read memory metrics\n");
        munmap(ptr, n_allocation * sizeof(long));
        return;
    }
    
    printf("\nVmSize After: %lu KB | VmRSS After: %lu KB\n", after.vm_size_kb, after.vm_rss_kb);

    char record[1024];
    int used = snprintf(record, sizeof(record),
        "How many intergers to allocate: %d\n"
        "Starting memory address: %p\n"
        "VmSize Before: %lu KB | VmRSS Before: %lu KB\n"
        "Printing the first 10 values in reverse order:\n",
        n_allocation, (void *)ptr, before.vm_size_kb, before.vm_rss_kb);

    for (long i=0; i<count; i++) {
        used += snprintf(record + used, sizeof(record) - used, "%ld\n", ptr[n_allocation - i - 1]);
    }

    snprintf(record + used, sizeof(record) - used,
        "\nVmSize After: %lu KB | VmRSS After: %lu KB",
        after.vm_size_kb, after.vm_rss_kb);

    write_logs("mmap", record);

    if (munmap(ptr, n_allocation * sizeof(long)) == -1) {
        perror("munmap failed");
        return;
    }
}
    
