/* Bagian Fiqhi - parser VmSize/VmRSS dari /proc/self/status untuk semua modul. */
#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

typedef struct {
    unsigned long vm_size_kb;
    unsigned long vm_rss_kb;
} MemoryMetrics;

/* 0 = berhasil, -1 = gagal; gunakan output hanya jika berhasil. */
int read_memory_metrics(MemoryMetrics *metrics);

#endif
