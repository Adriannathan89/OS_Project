AYO KERJA LEE

Kompilasi dan jalankan contoh input:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic scheduler.c -o a.out
./a.out < input.txt
```

Format input: jumlah proses, quantum Q1, quantum Q2, kemudian `AT BT queue`
untuk setiap proses. Jumlah proses, quantum, dan BT harus positif; AT harus
>= 0; queue harus 1, 2, atau 3.

Kode juga dapat dikompilasi dengan `g++ -std=c++17 scheduler.c -o a.out`.

Jalankan tes regresi (memerlukan Python 3, GCC, dan G++):

```sh
python3 -m unittest discover -s tests -v
```

Skenario pengujian MLQ dengan input/output terminal tersedia di
[`tests/unittest.txt`](tests/unittest.txt). Delapan input siap pakai di
`tests/inputs/` mencakup kondisi normal dengan quantum 1, 2, dan 4,
arrival tersebar dan CPU idle, burst time timpang, 12 proses, edge case,
serta satu kasus FCFS dengan perhitungan manual.

Contoh menjalankan dari direktori TK2:

```sh
./a.out < tests/inputs/01_normal_q1.txt
```

Dokumen menyertakan hasil MLQ yang diharapkan dan hasil aktual program.
Saat ini `main()` hanya mengeksekusi Q3 (FCFS), sehingga kasus yang memakai
Q1/Q2 belum lulus pengujian MLQ. Interval kasus manual Q3 sudah cocok dengan
output program; metrik TAT/WT/RT dihitung dari interval tersebut.
