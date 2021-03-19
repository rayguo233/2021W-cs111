CC = /usr/local/cs/gcc-9.3.0/bin/gcc
CFLAGS = -pthread -g -o
WARN = -Wall -Wextra
LIST_SRC = lab2_list.c SortedList.c
LIST_OUT = lab2_list
LIST_TEST = tests_list.sh
LIST_CSV = lab2b_list.csv
LIST_GRAPH = lab2_list.gp
LIST_PNG = lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png
TAR = lab2b-123456789.tar.gz
SUBMIT = $(LIST_SRC) $(LIST_CSV) $(LIST_GRAPH) $(LIST_TEST) $(LIST_PNG) \
		 Makefile README SortedList.h 
CHECK_SCR = P2B_check.sh

build: $(LIST_OUT)
tests: $(LIST_OUT) $(LIST_TEST)
	chmod +x $(LIST_TEST)
	./$(LIST_TEST)
profile:
	rm -f profile.gperf profile.out
	LD_PRELOAD=/usr/lib64/libprofiler.so \
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	/usr/local/cs/bin/pprof --text ./lab2_list ./raw.gperf > profile.out
	/usr/local/cs/bin/pprof --list=thread_worker ./lab2_list ./raw.gperf >> profile.out
	/usr/local/cs/bin/pprof --list=lock_general ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf
graphs: $(LIST_CSV)
	chmod +x $(LIST_GRAPH)
	./$(LIST_GRAPH)
dist: tests graphs profile $(TAR)
clean:
	rm -f $(ADD_OUT) $(LIST_OUT) $(TAR)
$(LIST_OUT): $(LIST_SRC)
	$(CC) $(WARN) $(CFLAGS) $(LIST_OUT) $(LIST_SRC)
$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT) profile.out
check: dist
	chmod +x $(CHECK_SCR)
	./$(CHECK_SCR) 123456789
cleanall:
	rm -f $(LIST_OUT) $(TAR) $(LIST_CSV) $(LIST_GRAPH)