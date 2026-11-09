#include <stddef.h>
#include <stdint.h>     
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <time.h>   
#include "variant8.h"  

#define FILE_SIZE_BYTES ((size_t)50 * 1024 * 1024)
#define INPUT_FILE_PATH "variant8_data.bin"       

typedef struct {
    uint64_t total_sum;   
    double elapsed_seconds; 
    long vmrss_delta_kb;      
} Result;




static int create_test_file(const char *path, size_t size){
    unsigned char buffer[4096];
    size_t total_written = 0;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = 1;
    }
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    while (total_written < size) {
        size_t amount = size - total_written;
        if (amount > sizeof(buffer)) amount = sizeof(buffer);
        size_t chunk_written = 0;
        while (chunk_written < amount) {
            ssize_t n = write(fd, buffer + chunk_written, amount - chunk_written);
            if (n <= 0) {
                if (n < 0)  perror("write");
                else fprintf(stderr, "write tidak menulis data\n");
                close(fd);
                return -1;
            }
            chunk_written += (size_t)n;
        }
        total_written += amount;
    }
    if (close(fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
} 



static long read_vm_rss_kb(void){
    FILE *file = fopen("/proc/self/status", "r");
    char line[256];
    long rss_kb = -1;
    if (file == NULL) {
        perror("fopen");
        return -1;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "VmRSS: %ld kB", &rss_kb) == 1) {
            fclose(file);
            return rss_kb;
        }
    }
    fclose(file);
    fprintf(stderr, "Baris VmRSS tidak ditemukan\n");
    return -1;
}

static int sum_with_read(const char *path, Result *result){
    long rss_before = read_vm_rss_kb();
    if (rss_before < 0) return -1;
    clock_t start = clock();
    if (start == (clock_t)-1) {
        fprintf(stderr, "Gagal membaca waktu awal\n");
        return -1;
    }
    unsigned char *buffer = malloc(FILE_SIZE_BYTES);
    if (buffer == NULL) {
        perror("malloc");
        return -1;
    }
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        free(buffer);
        return -1;
    }
    size_t total_read = 0;
    while (total_read < FILE_SIZE_BYTES) {
        ssize_t count = read(fd, buffer + total_read, FILE_SIZE_BYTES - total_read);

        if (count <= 0) {
            if (count < 0) perror("read");
            else fprintf(stderr, "File berakhir sebelum seluruh data terbaca\n");
            
            close(fd);
            free(buffer);
            return -1;
        }
        total_read += (size_t)count;
    }
    uint64_t sum = 0;
    for (size_t i = 0; i < FILE_SIZE_BYTES; i++) {
        sum += buffer[i];
    }
    clock_t end = clock();
    if (end == (clock_t)-1) {
        fprintf(stderr, "Gagal membaca waktu akhir\n");
        close(fd);
        free(buffer);
        return -1;
    }
    long rss_after = read_vm_rss_kb();
    if (rss_after < 0) {
        close(fd);
        free(buffer);
        return -1;
    }
    result->total_sum = sum;
    result->elapsed_seconds = (double)(end - start) / CLOCKS_PER_SEC;
    result->vmrss_delta_kb = rss_after - rss_before;
    close(fd);
    free(buffer);
    return 0;
}

static int sum_with_mmap(const char *path, Result *result){
    long rss_before = read_vm_rss_kb();
    if (rss_before < 0)  return -1;
    clock_t start = clock();
    if (start == (clock_t)-1) {
        fprintf(stderr, "Gagal membaca waktu awal\n");
        return -1;
    }
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    unsigned char *data = mmap(NULL, FILE_SIZE_BYTES, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    uint64_t sum = 0;
    for (size_t i = 0; i < FILE_SIZE_BYTES; i++) sum += data[i];
    clock_t end = clock();
    if (end == (clock_t)-1) {
        fprintf(stderr, "Gagal membaca waktu akhir\n");
        munmap(data, FILE_SIZE_BYTES);
        close(fd);
        return -1;
    }
    long rss_after = read_vm_rss_kb();
    if (rss_after < 0) {
        munmap(data, FILE_SIZE_BYTES);
        close(fd);
        return -1;
    result->total_sum = sum;
    result->elapsed_seconds = (double)(end - start) / CLOCKS_PER_SEC;
    result->vmrss_delta_kb = rss_after - rss_before;
    munmap(data, FILE_SIZE_BYTES);
    close(fd);
    return 0;
}


static void print_result(const char *method, Result *result){
    printf("[%s]\n", method);
    printf("Waktu Eksekusi : %.6f detik\n", result->elapsed_seconds);
    printf("Total Sum      : %llu\n", (unsigned long long)result->total_sum);
    printf("Lonjakan VmRSS : %ld KB\n\n", result->vmrss_delta_kb);
}

static void compare_results(Result *read_result, Result *mmap_result){
    if (read_result->total_sum == mmap_result->total_sum) printf("[Kesimpulan] Total sum sama (%llu), kedua metode valid.\n", (unsigned long long)read_result->total_sum);
    else printf("[Kesimpulan] Total sum berbeda.\n");
}

void run_variant8(void){
    Result read_result = {0};
    Result mmap_result = {0};

    printf("--- Varian 8: Pemetaan Berkas vs System Call I/O ---\n");
    printf("[Info] Menyiapkan file %zu bytes...\n\n", FILE_SIZE_BYTES);

    if (create_test_file(INPUT_FILE_PATH, FILE_SIZE_BYTES) != 0) {
        fprintf(stderr, "Gagal membuat file input\n");
        unlink(INPUT_FILE_PATH);
        return;
    }

    if (sum_with_read(INPUT_FILE_PATH, &read_result) != 0) {
        fprintf(stderr, "Metode read() gagal\n");
        unlink(INPUT_FILE_PATH);
        return;
    }

    print_result("Metode 1: read() & malloc", &read_result);

    if (sum_with_mmap(INPUT_FILE_PATH, &mmap_result) != 0) {
        fprintf(stderr, "Metode mmap() gagal\n");
        unlink(INPUT_FILE_PATH);
        return;
    }

    print_result("Metode 2: mmap()", &mmap_result);
    compare_results(&read_result, &mmap_result);

    if (unlink(INPUT_FILE_PATH) != 0) {
        perror("unlink");
    }

}


