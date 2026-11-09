/* Bagian Fiqhi - parser VmSize/VmRSS dari /proc/self/status untuk semua modul. */
#include "memory_monitor.h"
#include <stdio.h>

int read_memory_metrics(MemoryMetrics *metrics) {
    /* TODO: Baca VmSize dan VmRSS dari /proc/self/status. */
    FILE* status_vm = fopen("/proc/self/status", "r"); 
    if (status_vm == NULL) {
        return -1;
    }

    char line[256];
    long vm_size = -1, vm_rss = -1;

    while (fgets(line, sizeof(line), status_vm)) {
        if (sscanf(line, "VmSize: %ld", &vm_size) == 1) {
            continue;
        }
        if (sscanf(line, "VmRSS: %ld", &vm_rss) == 1) {
            continue;
        }
    }

    if (vm_size == -1 || vm_rss == -1) {
        fclose(status_vm);
        return -1;
    }
    metrics->vm_size_kb = vm_size;
    metrics->vm_rss_kb = vm_rss;

    fclose(status_vm);
    
    return 0;
}
