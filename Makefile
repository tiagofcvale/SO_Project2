CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pthread -D_XOPEN_SOURCE=700
LIBS = -lrt

SRCDIR = src
OBJDIR = obj
TARGET = httpserver
SOURCES = \
    $(SRCDIR)/main.c \
    $(SRCDIR)/master.c \
    $(SRCDIR)/worker.c \
    $(SRCDIR)/thread_pool.c \
    $(SRCDIR)/http.c \
    $(SRCDIR)/cache.c \
    $(SRCDIR)/config.c \
    $(SRCDIR)/logger.c \
    $(SRCDIR)/global.c


OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)

.PHONY: clean
