/* Bagian Hisyam - menu utama dan integrasi semua modul. */
#include <stdio.h>
#include "banner.h"
#include "malloc_module.h"
#include "mmap_module.h"
#include "variant8.h"

int main(void)
{
    print_banner();
    while (1) {
        printf("\n=== Kelompok B08 - Tugas Memori ===\n");
        printf("1. Allocate dengan malloc/free\n");
        printf("2. Allocate dengan mmap/munmap\n");
        printf("3. Eksekusi fitur variasi kelompok\n");
        printf("4. Exit\n");
        printf("Input: ");
        
        int choice;
        scanf("%d", &choice);

        if (choice == 1) {
            run_malloc_experiment();
        } else if (choice == 2) {
            run_mmap_experiment();
        } else if (choice == 3) {
            run_variant8();
        } else if (choice == 4) {
            break;
        } else {
            puts("Input hanya berupa integer 1-4.");
        }
    }
    return 0;
}
