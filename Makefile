CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -pthread
LDFLAGS = -lreadline -pthread

EXEC = biceps
EXEC_MEM = biceps-memory-leaks

SRC_BICEPS = bicep.c gescom.c creme.c serveur_udp.c serveur_tcp.c

all: $(EXEC)

$(EXEC): $(SRC_BICEPS)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC_BICEPS) $(LDFLAGS)

memory-leak: $(SRC_BICEPS)
	$(CC) $(CFLAGS) -O0 -o $(EXEC_MEM) $(SRC_BICEPS) $(LDFLAGS)

clean:
	rm -f $(EXEC) $(EXEC_MEM)

.PHONY: all clean memory-leak