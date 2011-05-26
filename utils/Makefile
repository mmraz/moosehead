CC      = gcc
PROF    = -O -g
C_FLAGS =  -Wall $(PROF)
L_FLAGS =  $(PROF)

O_FILES = count.o read.o

counter: $(O_FILES)
	rm -f counter 
	$(CC) $(L_FLAGS) -o counter $(O_FILES)

.c.o: count.h
	$(CC) -c $(C_FLAGS) $<
