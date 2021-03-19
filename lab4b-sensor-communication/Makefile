
CC = gcc
CFLAGS = -g -o
WARN = -Wall -Wextra
SRC = lab4b.c
OUT = lab4b
TAR = lab4b-123456789.tar.gz
SUBMIT = Makefile README $(SRC) $(SMOKE_T)
TEST_SCR = P4B_check.sh
SMOKE_T = smoketest.sh

default: $(OUT)

dist: $(TAR)

clean:
	rm -rf $(OUT) $(TAR) CS111_test.rayguo functions.sh

check: $(OUT) $(SMOKE_T)
	chmod +x ./$(SMOKE_T)
	./$(SMOKE_T)	

$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)

$(OUT): $(SRC)
	@if [ `uname -a | grep -c "armv7l"` -ne 0 ]; then \
		$(CC) $(WARN) -lmraa $(CFLAGS) $(OUT) $(SRC) -lm;\
	else \
		$(CC) $(WARN) -DDUMMY $(CFLAGS) $(OUT) $(SRC) -lm;\
	fi
	chmod +x $(OUT)

test: $(TAR) $(TEST_SCR)
	chmod +x $(TEST_SCR)
	./$(TEST_SCR) 123456789
	
r: $(OUT)
	./$(OUT)
