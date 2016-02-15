CC = mpicc
CFLAGS = -Wall -O2 -lgmp -lm
factor: factor.o
        $(CC) -o $@ $? -lgmp -lm
clean:
        $(RM) *.o *~ 