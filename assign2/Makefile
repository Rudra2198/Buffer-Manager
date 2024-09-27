# Variables
CC = gcc
CFLAGS = -Wall -g

# Object files for the tests
OBJ1 = buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o test_assign2_1.o
OBJ2 = buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o test_assign2_2.o

# Targets
all: run_test_1 run_test_2

# Compilation rules for test 1
test1: $(OBJ1)
	$(CC) $(CFLAGS) -o test1 $(OBJ1)

buffer_mgr.o: buffer_mgr.c buffer_mgr.h
	$(CC) $(CFLAGS) -c buffer_mgr.c

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h
	$(CC) $(CFLAGS) -c buffer_mgr_stat.c

dberror.o: dberror.c dberror.h
	$(CC) $(CFLAGS) -c dberror.c

storage_mgr.o: storage_mgr.c storage_mgr.h
	$(CC) $(CFLAGS) -c storage_mgr.c

test_assign2_1.o: test_assign2_1.c
	$(CC) $(CFLAGS) -c test_assign2_1.c

# Rule to run test 1
run_test_1: test1
	./test1

# Compilation rules for test 2
test2: $(OBJ2)
	$(CC) $(CFLAGS) -o test2 $(OBJ2)

test_assign2_2.o: test_assign2_2.c
	$(CC) $(CFLAGS) -c test_assign2_2.c

# Rule to run test 2
run_test_2: test2
	./test2

# Clean up object and executable files
clean:
	rm -f *.o test1 test2
