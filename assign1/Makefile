# WORK DONE by ENOCH ASHONG (A20555274)
# Compiler
# Compiler
CC = gcc
CFLAGS = -Wall -g

# Source files
SRCS = dberror.c storage_mgr.c test_assign1_1.c
OBJS = $(SRCS:.c=.o)

# Executable
TARGET = test_assign1

# Rule for building the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule for compiling .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean up object files and the executable
clean:
	rm -f $(OBJS) $(TARGET)

# Run the tests
run: $(TARGET)
	./$(TARGET)
