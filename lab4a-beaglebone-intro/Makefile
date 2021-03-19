TAR = lab4a-123456789.tar.gz
SUBMIT = my_beaglebone.jpg connected.png copy.png build_run.png\
		 git.png Makefile README
CHECK = P4A_check.sh

default: $(TAR)

$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)

check: $(CHECK)
	./$(CHECK) 123456789


