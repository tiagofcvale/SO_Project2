CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pthread -D_XOPEN_SOURCE=700
LIBS = -lrt

SRCDIR = src
OBJDIR = obj
TARGET = httpserver
SOURCES = src/main.c src/master.c src/worker.c src/thread_pool.c \
          src/http.c src/config.c src/logger.c src/global.c

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
