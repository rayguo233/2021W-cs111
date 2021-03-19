
CC = /usr/local/cs/gcc-9.3.0/bin/gcc
CFLAGS = -lz -o 
WARN = -Wall -Wextra
C_SRC = lab1b-client.c
C_OUT = lab1b-client
S_SRC = lab1b-server.c
S_OUT = lab1b-server
TAR = lab1b-123456789.tar.gz
SUBMIT = $(C_SRC) $(S_SRC) Makefile README
PORT = 2222
CHECK_SCR = P1B_check.sh

default: $(C_OUT) $(S_OUT)

$(C_OUT): $(C_SRC)
	$(CC) $(WARN) $(CFLAGS) $(C_OUT) $(C_SRC) 
	
$(S_OUT): $(S_SRC)
	$(CC) $(WARN) $(CFLAGS) $(S_OUT) $(S_SRC) 

clean:
	rm -f $(S_OUT) $(C_OUT) $(TAR) log1b.txt

dist: $(TAR)

$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)

# debug/testing options

check: dist
	./$(CHECK_SCR) 123456789

c: $(C_OUT)

rc: $(C_OUT)
	./$(C_OUT) --port=$(PORT)

s: $(S_OUT)

rs: $(S_OUT)
	./$(S_OUT) --port=$(PORT)

log: $(C_OUT)
	./$(C_OUT) --port=$(PORT) --log log1b.txt

cs: $(S_OUT)
	./$(S_OUT) --port=$(PORT) --compress

cc: $(C_OUT)
	./$(C_OUT) --port=$(PORT) --log log1b.txt --compress
