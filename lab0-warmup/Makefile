CC = /usr/local/cs/gcc-9.3.0/bin/gcc
CFLAGS = -o
DBCFLAGS = -g -o
WARN = -Wall -Wextra
SRC = lab0.c
OUT = lab0
DOUT = lab0deb
SCRIPT = test.sh
TAR = lab0-123456789.tar.gz
SUBMIT = lab0.c Makefile README test.sh backtrace.png breakpoint.png

default: $(OUT)

$(OUT): $(SRC) 
	$(CC) $(SRC) $(CFLAGS) $(OUT) $(WARN)
	
clean:
	rm -f $(OUT) $(DOUT) $(TAR)

check: $(SCRIPT)
	./$(SCRIPT)

dist: $(TAR)

$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)

debug: $(DOUT)

$(DOUT): $(SRC)
	$(CC) $(DBCFLAGS) $(DOUT) $(SRC) $(WARN)	
