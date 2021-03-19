#!/usr/local/cs/bin/bash

rm -rf lab2b_list.csv

# lab2b_1, lab2b_2
for NTH in 1, 2, 4, 8, 12, 16, 24
do
    ./lab2_list --threads=${NTH} --iterations=1000 --sync=m >> lab2b_list.csv
    ./lab2_list --threads=${NTH} --iterations=1000 --sync=s >> lab2b_list.csv
done

# lab2b_3

for NTH in 1, 4, 8, 12, 16 
do
    (for NIT in 1, 2, 4, 8, 16
    do
        ./lab2_list --threads=${NTH} --iterations=${NIT} --yield=id --lists=4 >> lab2b_list.csv || true
    done)   
done
for NTH in 1, 4, 8, 12, 16 
do
    (for NIT in 10, 20, 40, 80
    do
        ./lab2_list --threads=${NTH} --iterations=${NIT} --yield=id --lists=4 --sync=m >> lab2b_list.csv
        ./lab2_list --threads=${NTH} --iterations=${NIT} --yield=id --lists=4 --sync=s >> lab2b_list.csv
    done)   
done

# lab2b_4, lab2b_5
for NTH in 1, 2, 4, 8, 12
do
    (for NLIST in 4, 8, 16
    do
        ./lab2_list --threads=${NTH} --iterations=1000 --lists=${NLIST} --sync=m >> lab2b_list.csv
        ./lab2_list --threads=${NTH} --iterations=1000 --lists=${NLIST} --sync=s >> lab2b_list.csv
    done)   
done