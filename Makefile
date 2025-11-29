CC = cc
# Adicionado -lrt para shared memory e semaphores
CFLAGS = -Wall -Wextra -Werror -std=c99 -pthread -D_XOPEN_SOURCE=700 -g
LIBS = -lrt

SRCDIR = src
OBJDIR = obj

# ADICIONADO: shared_mem.c e semaphores.c
SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/master.c \
          $(SRCDIR)/worker.c \
          $(SRCDIR)/thread_pool.c \
          $(SRCDIR)/http.c \
          $(SRCDIR)/cache.c \
          $(SRCDIR)/stats.c \
          $(SRCDIR)/global.c \
          $(SRCDIR)/config.c \
          $(SRCDIR)/logger.c \
          $(SRCDIR)/shared_mem.c \
          $(SRCDIR)/semaphores.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = httpserver

# ---------------------------------------------------------
# Regra principal
# ---------------------------------------------------------
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# ---------------------------------------------------------
# Regras para compilar .c -> .o
# ---------------------------------------------------------
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------
# Limpeza
# ---------------------------------------------------------
clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: clean