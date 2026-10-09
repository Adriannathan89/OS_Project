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
Testcase 01–08 sudah cocok dengan simulasi MLQ terpisah, termasuk Gantt,
CT/TAT/WT/RT untuk seluruh 47 proses, rata-rata, utilization, throughput,
context switch, preemption, dan state transitions. Hasil hitungan manual
TC08 juga cocok dengan tabel dan rata-rata yang dicetak program.

Output lengkap tiap testcase tersimpan di [`tests/outputs/`](tests/outputs/)
dengan nama yang sama seperti inputnya. Ringkasan verifikasi dan SHA256
source yang diuji tersedia di [`tests/verification.txt`](tests/verification.txt).
