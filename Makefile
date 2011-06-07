CFLAGS = -Wall -g -O3 -funroll-loops -fwhole-program

rc2-40-cbc: rc2-40-cbc.c rc2.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	$(RM) rc2-40-cbc

.PHONY: clean
