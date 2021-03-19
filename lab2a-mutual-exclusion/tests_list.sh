#!/usr/local/cs/bin/bash
rm -rf lab2_list.csv

# list-1
for NIT in 10, 100, 1000, 10000, 20000
do
    ./lab2_list --iterations=${NIT} >> lab2_list.csv || true
done

# list-2
for NTH in 2, 4, 8, 12
do
    (for NIT in 1, 10,100,1000
    do
        ./lab2_list --threads=${NTH} --iterations=${NIT} >> lab2_list.csv || true
    done)   
done
for NTH in 2, 4, 8, 12
do
    (for NIT in 1, 2, 4, 8, 16, 32
    do
        ./lab2_list --threads=${NTH} --iterations=${NIT} --yield=i >> lab2_list.csv || true
        ./lab2_list --threads=${NTH} --iterations=${NIT} --yield=d >> lab2_list.csv || true
        ./lab2_list --threads=${NTH} --iterations=${NIT} --yield=il >> lab2_list.csv || true
        ./lab2_list --threads=${NTH} --iterations=${NIT} --yield=dl >> lab2_list.csv || true
    done)   
done

# list-3
for SYNC in m s
do
    (for YLD in i d il dl
    do
        ./lab2_list --threads=12 --iterations=1 --sync=${SYNC} --yield=${YLD} >> lab2_list.csv || true
        ./lab2_list --threads=12 --iterations=2 --sync=${SYNC} --yield=${YLD} >> lab2_list.csv || true
        ./lab2_list --threads=12 --iterations=4 --sync=${SYNC} --yield=${YLD} >> lab2_list.csv || true
        ./lab2_list --threads=12 --iterations=8 --sync=${SYNC} --yield=${YLD} >> lab2_list.csv || true
        ./lab2_list --threads=12 --iterations=16 --sync=${SYNC} --yield=${YLD} >> lab2_list.csv || true
        ./lab2_list --threads=12 --iterations=32 --sync=${SYNC} --yield=${YLD} >> lab2_list.csv || true
    done)   
done

# list-4
for SYNC in m s
do
    (for NTH in 1, 2, 4, 8, 12, 16, 24
    do
        ./lab2_list --threads=${NTH} --iterations=1000 --sync=${SYNC} >> lab2_list.csv || true
    done)   
done