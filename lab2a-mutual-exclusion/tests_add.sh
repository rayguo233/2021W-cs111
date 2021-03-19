#!/usr/local/cs/bin/bash

rm -rf lab2_add.csv

# add-1
for NTH in 2, 4, 8, 12
do
    (for NIT in 10, 20, 40, 80, 100, 1000, 10000, 100000
    do
        ./lab2_add --threads=${NTH} --iterations=${NIT} --yield >> lab2_add.csv
    done)
done

# add-2
for NTH in 2, 8
do
    (for NIT in 100, 1000, 10000, 100000
    do
        ./lab2_add --threads=${NTH} --iterations=${NIT} >> lab2_add.csv
    done)
done

# add-3
for NIT in 100, 1000, 10000, 100000
do
    ./lab2_add --threads=1 --iterations=${NIT} >> lab2_add.csv
done

# add-4
for NTH in 2, 4, 8, 12
do
    ./lab2_add --threads=${NTH} --iterations=10000 --yield --sync=m >> lab2_add.csv
    ./lab2_add --threads=${NTH} --iterations=10000 --yield --sync=c >> lab2_add.csv
    ./lab2_add --threads=${NTH} --iterations=1000 --yield --sync=s >> lab2_add.csv
done

# add-5
for NTH in 1, 2, 4, 8, 12
do
    ./lab2_add --threads=${NTH} --iterations=10000 >> lab2_add.csv
    ./lab2_add --threads=${NTH} --iterations=10000 --sync=m >> lab2_add.csv
    ./lab2_add --threads=${NTH} --iterations=10000 --sync=c >> lab2_add.csv
    ./lab2_add --threads=${NTH} --iterations=10000 --sync=s >> lab2_add.csv
done