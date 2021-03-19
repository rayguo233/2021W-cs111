CC = /usr/local/cs/gcc-9.3.0/bin/gcc
CFLAGS = -pthread -o 
WARN = -Wall -Wextra
ADD_SRC = lab2_add.c
ADD_OUT = lab2_add
ADD_TEST = tests_add.sh
ADD_CSV = lab2_add.csv
ADD_GRAPH = lab2_add-1.png lab2_add-2.png lab2_add-3.png\
			lab2_add-4.png lab2_add-5.png
LIST_SRC = lab2_list.c SortedList.c
LIST_OUT = lab2_list
LIST_TEST = tests_list.sh
LIST_CSV = lab2_list.csv
LIST_GRAPH = lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
TAR = lab2a-123456789.tar.gz
SUBMIT = $(ADD_SRC) $(LIST_SRC) Makefile README SortedList.h $(LIST_CSV)\
		 $(ADD_CSV) $(ADD_GRAPH) $(LIST_GRAPH) $(ADD_TEST) $(LIST_TEST)\
		 ./lab2_list.gp ./lab2_add.gp
CHECK_SCR = P2A_check.sh

build: $(ADD_OUT) $(LIST_OUT)
tests: $(ADD_OUT) $(LIST_OUT) $(LIST_TEST) $(ADD_TEST)
	./$(ADD_TEST)
	./$(LIST_TEST)
graphs: plota plotl
dist: $(ADD_OUT) $(LIST_OUT)
	chmod 744 $(ADD_TEST)
	chmod 744 lab2_add.gp
	chmod 744 $(LIST_TEST)
	chmod 744 lab2_list.gp
	./$(ADD_TEST)
	./lab2_add.gp
	./$(LIST_TEST)
	./lab2_list.gp
	tar -czvf $(TAR) $(SUBMIT)
clean:
	rm -f $(ADD_OUT) $(LIST_OUT) $(TAR)
plota: $(ADD_CSV) ./lab2_add.gp
	chmod 744 lab2_add.gp
	./lab2_add.gp
plotl: $(LIST_CSV) ./lab2_list.gp
	chmod 744 lab2_list.gp
	./lab2_list.gp
$(ADD_OUT): $(ADD_SRC)
	$(CC) $(WARN) $(CFLAGS) $(ADD_OUT) $(ADD_SRC)
$(LIST_OUT): $(LIST_SRC)
	$(CC) $(WARN) $(CFLAGS) $(LIST_OUT) $(LIST_SRC)
seea: $(ADD_CSV)
	cat $(ADD_CSV)
seel: $(LIST_CSV)
	cat $(LIST_CSV)
$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)

check: dist
	./$(CHECK_SCR) 123456789

cleanall:
	rm -f $(ADD_OUT) $(LIST_OUT) $(TAR) $(LIST_CSV)\
		 	$(ADD_CSV) $(ADD_GRAPH) $(LIST_GRAPH)