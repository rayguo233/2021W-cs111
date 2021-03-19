#! /usr/local/cs/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png
set title "Lab2b-1: Aggregate Throughput for Mutex and Spin-Lock"
set xlabel "Threads"
set xrange [1:]
set ylabel "Operations per second"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep -E 'list-none-m,[1-9]+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'mutex' with linespoints lc rgb 'green', \
     "< grep -E 'list-none-s,[1-9]+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'spin-lock' with linespoints lc rgb 'red'

# lab2b_2.png
set title "Lab2b-2: Average Wait-For-Lock for Mutex and Spin-Lock"
set xlabel "Threads"
set xrange [1:]
set ylabel "Average wait-for-lock"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep -E 'list-none-m,[1-9]+,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'mutex' with linespoints lc rgb 'green', \
     "< grep -E 'list-none-s,[1-9]+,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'spin-lock' with linespoints lc rgb 'red'

# lab2b_3.png
set title "Lab2b-3: Successful Runs with 4 Sublists"
set xlabel "Threads"
set ylabel "Iterations"
set output 'lab2b_3.png'

plot \
     "< grep -E 'list-id-none' lab2b_list.csv" using ($2):($3) \
	title 'no protection' with points lc rgb 'green', \
     "< grep -E 'list-id-m' lab2b_list.csv" using ($2):($3) \
	title 'mutex' with points lc rgb 'red', \
     "< grep -E 'list-id-s' lab2b_list.csv" using ($2):($3) \
	title 'spin-lock' with points lc rgb 'blue'

# lab2b_4.png
set title "Lab2b-4: Sublists with Mutex"
set xlabel "Threads"
set ylabel "Operations per second"
set output 'lab2b_4.png'

plot \
     "< grep -E 'list-none-m,([1-9]|12),1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'green', \
     "< grep -E 'list-none-m,[1-9]+,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 list' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-m,[1-9]+,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 list' with linespoints lc rgb 'blue', \
     "< grep -E 'list-none-m,[1-9]+,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 list' with linespoints lc rgb 'yellow'

# lab2b_5.png
set title "Lab2b-5: Sublists with Spin-Lock"
set xlabel "Threads"
set ylabel "Operations per second"
set output 'lab2b_5.png'

plot \
     "< grep -E 'list-none-s,([1-9]|12),1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'green', \
     "< grep -E 'list-none-s,[1-9]+,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 list' with linespoints lc rgb 'red', \
     "< grep -E 'list-none-s,[1-9]+,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 list' with linespoints lc rgb 'blue', \
     "< grep -E 'list-none-s,[1-9]+,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 list' with linespoints lc rgb 'yellow'