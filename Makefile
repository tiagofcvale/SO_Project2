CC = cc
CFLAGS = -Wall -Wextra -std=c99 -pthread
SRCDIR = src
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/master.c $(SRCDIR)/config.c $(SRCDIR)/logger.c
TARGET = httpserver
OBJDIR = obj

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: clean