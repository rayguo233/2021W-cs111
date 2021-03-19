CC = gcc
CFLAGS = -g -o
WARN = -Wall -Wextra
TCP_SRC = lab4c_tcp.c
TLS_SRC = lab4c_tls.c
TCP_OUT = lab4c_tcp
TLS_OUT = lab4c_tls
SRC = $(TCP_SRC) $(TLS_SRC)
OUT = $(TCP_OUT) $(TLS_OUT)
TAR = lab4c-123456789.tar.gz
SUBMIT = Makefile README $(SRC)
TEST_SCR = P4C_check.sh

default: $(OUT)

dist: $(TAR)

clean:
	rm -rf $(OUT) $(TAR) CS111_test.rayguo functions.sh

check: $(TAR) $(TEST_SCR)
	chmod +x $(TEST_SCR)
	./$(TEST_SCR) 123456789

$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)

$(TCP_OUT): $(TCP_SRC)
	@if [ `uname -a | grep -c "armv7l"` -ne 0 ]; then \
		$(CC) $(WARN) -lmraa $(CFLAGS) $(TCP_OUT) $(TCP_SRC) -lm;\
	else \
		$(CC) $(WARN) -DDUMMY $(CFLAGS) $(TCP_OUT) $(TCP_SRC) -lm;\
	fi
	chmod +x $(TCP_OUT)

$(TLS_OUT): $(TLS_SRC)
	@if [ `uname -a | grep -c "armv7l"` -ne 0 ]; then \
		$(CC) $(WARN) -lmraa -lssl -lcrypto $(CFLAGS) $(TLS_OUT) $(TLS_SRC) -lm;\
	else \
		$(CC) $(WARN) -DDUMMY -lssl -lcrypto $(CFLAGS) $(TLS_OUT) $(TLS_SRC) -lm;\
	fi
	chmod +x $(TLS_OUT)
	
r: $(TLS_OUT)
	./$(TLS_OUT) --host=111.179.192.116 19000 --id=123456789 --log=log

rr: $(TCP_OUT)
	./$(TCP_OUT) --host=111.179.192.116 18000 --id=123456789 --log=log
