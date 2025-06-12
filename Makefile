CC = gcc
CFLAGS = -Wall -pthread

SRCDIR = src
BINDIR = .

TARGETS = $(BINDIR)/server $(BINDIR)/client

all: $(TARGETS)

$(BINDIR)/server: $(SRCDIR)/server.c $(SRCDIR)/handle_result.c
	$(CC) $(CFLAGS) -o $@ $^

$(BINDIR)/client: $(SRCDIR)/client.c $(SRCDIR)/handle_result.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BINDIR)/server $(BINDIR)/client

