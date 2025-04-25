# Makefile for the Multi-Process IPC Example

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g  # -g for debugging symbols
LDFLAGS = -lrt # Link Realtime Extensions library for message queues (-lm might be needed if using math functions)

# Executables
TARGETS = process1 process2 process3 process4 process5

# Source files
SOURCES = process1.c process2.c process3.c process4.c process5.c

# Object files (optional, could compile directly)
# OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGETS)
# Rule to build each process
process1: process1.c common.h
	$(CC) $(CFLAGS) process1.c -o process1 $(LDFLAGS)
process2: process2.c common.h
	$(CC) $(CFLAGS) process2.c -o process2 $(LDFLAGS)
process3: process3.c common.h
	$(CC) $(CFLAGS) process3.c -o process3 $(LDFLAGS)
process4: process4.c common.h
	$(CC) $(CFLAGS) process4.c -o process4 $(LDFLAGS)
process5: process5.c common.h
	$(CC) $(CFLAGS) process5.c -o process5 $(LDFLAGS)
# Clean up executable files and potential IPC remnants
clean:
	rm -f $(TARGETS) *.o core.* core
	rm -f /tmp/p5_fifo /tmp/p5_socket
	# Message queues are in /dev/mqueue, need root or careful permissions to remove
	# Use `ipcrm -q <id>` or `rm /dev/mqueue/p5_mq` if needed
	@echo "Cleaned executables. IPC remnants might need manual removal:"
	@echo "  FIFO: rm /tmp/p5_fifo"
	@echo "  Socket: rm /tmp/p5_socket"
	@echo "  MsgQueue: rm /dev/mqueue/p5_mq (requires privileges)"
	@echo "  Log file: rm process5_log.txt (or specified name)"


.PHONY: all clean
