CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pthread -D_XOPEN_SOURCE=700
LIBS = -lrt

SRCDIR = src
OBJDIR = obj

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/master.c \
          $(SRCDIR)/worker.c \
          $(SRCDIR)/thread_pool.c \
          $(SRCDIR)/http.c \
          $(SRCDIR)/cache.c \
          $(SRCDIR)/stats.c \
          $(SRCDIR)/global.c \
          $(SRCDIR)/config.c \
          $(SRCDIR)/logger.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = httpserver


# ---------------------------------------------------------
# Regra principal
# ---------------------------------------------------------
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)


# ---------------------------------------------------------
# Regras para compilar .c → .o
# ---------------------------------------------------------
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@


# ---------------------------------------------------------
# Limpeza
# ---------------------------------------------------------
clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: clean
