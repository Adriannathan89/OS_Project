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
