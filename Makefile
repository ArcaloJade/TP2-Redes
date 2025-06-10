# CC      = gcc
# CFLAGS  = -Wall -Wextra -O2
# SRC     = src
# OBJS_CL = $(SRC)/client.o $(SRC)/util.o
# OBJS_SV = $(SRC)/server.o $(SRC)/util.o

# all: tpd-client tpd-server

# tpd-client: $(OBJS_CL)
# 	$(CC) $(CFLAGS) -o $@ $^

# tpd-server: $(OBJS_SV)
# 	$(CC) $(CFLAGS) -o $@ $^

# $(SRC)/%.o: $(SRC)/%.c $(SRC)/common.h
# 	$(CC) $(CFLAGS) -c -o $@ $<

# clean:
# 	rm -f $(SRC)/*.o tpd-client tpd-server

# ## Forma de probarlo:
# # $ make                     # compila ambos binarios
# # $ ./tpd-server &           # lanza el servidor (queda en background)
# # $ ./tpd-client 127.0.0.1   # mide bajada contra loopback (Cambiar 127.0.0.1 por la IP del servidor para probar por red real.)
# # Download: 800.00 Mb/s      # ejemplo

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

