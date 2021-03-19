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

rm -rf lab2_list.csv

# list-1
for NIT in 10, 100, 1000, 10000, 20000
do
    ./lab2_list --iterations=${NIT} >> lab2_list.csv
done

# list-2
for NTH in 2, 4, 8, 12
do
    (for NIT in 1, 10,100,1000
    do
        ./lab2_list --threads=${NTH} --iterations=${NIT} >> lab2_list.csv
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
