#include <stdio.h>
#include <stdbool.h>

int main() {
    double memory_usage = 0.0;
    double load_per_core = 0.0;

    // Baca input dari stdin
    if(scanf("%lf %lf", &memory_usage, &load_per_core) != 2) {
        fprintf(stderr, "Error: Input seharusnya hanya 2 angka\n");
        return 1;
    }

    bool is_failed = false;
    bool is_warned = false;
    // hitung metric untuk memory usage dari stdin sysinfo.sh
    if(memory_usage >= 90.0) {
        printf("FAIL");
        is_failed = true;
    } else if(memory_usage >= 75.0) {
        printf("WARN");
        is_warned = true;
    } else {
        printf("PASS");
    }

    // hitung metric untuk load avg vs jumlah core dari stdin sysinfo.sh
    if(load_per_core > 2.0) {
        printf(" FAIL");
        is_failed = true;
    } else if(load_per_core > 1.0) {
        printf(" WARN");
        is_warned = true;
    } else {
        printf(" PASS");
    }

    printf("\n");

    if(is_failed) {
        return 2;
    } else if(is_warned) {
        return 1;
    } else {
        return 0;
    }
}