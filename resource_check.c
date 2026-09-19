#include <stdio.h>

int main() {
    double memory_usage = 0.0;
    double load_per_core = 0.0;

    if(scanf("%lf %lf", &memory_usage, &load_per_core) != 2) {
        fprintf(stderr, "Error: Input seharusnya hanya 2 angka\n");
        return 1;
    }

    // hitung metric untuk memory usage dari stdin sysinfo.sh
    if(memory_usage >= 90.0) {
        printf("FAIL");
    } else if(memory_usage >= 75.0) {
        printf("WARN");
    } else {
        printf("PASS");
    }

    // hitung metric untuk load avg vs jumlah core dari stdin sysinfo.sh
    if(load_per_core > 2.0) {
        printf(" FAIL");
    } else if(load_per_core > 1.0) {
        printf(" WARN");
    } else {
        printf(" PASS");
    }

    printf("\n");
}