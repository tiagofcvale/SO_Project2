CC = gcc
CFLAGS = -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -g -Isrc
LDFLAGS = -pthread -lrt -lssl -lcrypto

# Diretórios
SRC_DIR = src
BUILD_DIR = build

TARGET = server

# Ficheiros fonte na pasta src/ (ADICIONADO ssl.c)
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/master.c $(SRC_DIR)/worker.c $(SRC_DIR)/http.c \
       $(SRC_DIR)/thread_pool.c $(SRC_DIR)/cache.c $(SRC_DIR)/logger.c $(SRC_DIR)/stats.c \
       $(SRC_DIR)/config.c $(SRC_DIR)/shared_mem.c $(SRC_DIR)/semaphores.c $(SRC_DIR)/global.c \
       $(SRC_DIR)/ssl.c

# Objetos na pasta build/
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all run test loadtest valgrind helgrind clean distclean help setup_www

# Regra padrão
all: $(BUILD_DIR) $(TARGET) setup_www

# Create build directory if it doesn't exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build is completed: $(TARGET)"

# Compile each .o file from .c
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Explicit dependencies
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/config.h $(SRC_DIR)/logger.h $(SRC_DIR)/stats.h $(SRC_DIR)/cache.h $(SRC_DIR)/master.h
$(BUILD_DIR)/master.o: $(SRC_DIR)/master.c $(SRC_DIR)/master.h $(SRC_DIR)/config.h $(SRC_DIR)/logger.h $(SRC_DIR)/worker.h $(SRC_DIR)/shared_mem.h $(SRC_DIR)/semaphores.h $(SRC_DIR)/ssl.h
$(BUILD_DIR)/worker.o: $(SRC_DIR)/worker.c $(SRC_DIR)/worker.h $(SRC_DIR)/config.h $(SRC_DIR)/thread_pool.h $(SRC_DIR)/shared_mem.h $(SRC_DIR)/semaphores.h $(SRC_DIR)/ssl.h
$(BUILD_DIR)/http.o: $(SRC_DIR)/http.c $(SRC_DIR)/http.h $(SRC_DIR)/config.h $(SRC_DIR)/logger.h $(SRC_DIR)/cache.h $(SRC_DIR)/stats.h $(SRC_DIR)/shared_mem.h $(SRC_DIR)/semaphores.h
$(BUILD_DIR)/thread_pool.o: $(SRC_DIR)/thread_pool.c $(SRC_DIR)/thread_pool.h $(SRC_DIR)/http.h
$(BUILD_DIR)/cache.o: $(SRC_DIR)/cache.c $(SRC_DIR)/cache.h
$(BUILD_DIR)/logger.o: $(SRC_DIR)/logger.c $(SRC_DIR)/logger.h $(SRC_DIR)/config.h
$(BUILD_DIR)/stats.o: $(SRC_DIR)/stats.c $(SRC_DIR)/stats.h
$(BUILD_DIR)/config.o: $(SRC_DIR)/config.c $(SRC_DIR)/config.h
$(BUILD_DIR)/shared_mem.o: $(SRC_DIR)/shared_mem.c $(SRC_DIR)/shared_mem.h $(SRC_DIR)/connection_queue.h $(SRC_DIR)/stats.h
$(BUILD_DIR)/semaphores.o: $(SRC_DIR)/semaphores.c $(SRC_DIR)/semaphores.h
$(BUILD_DIR)/global.o: $(SRC_DIR)/global.c $(SRC_DIR)/global.h
$(BUILD_DIR)/ssl.o: $(SRC_DIR)/ssl.c $(SRC_DIR)/ssl.h

# Create www directory structure and example pages
setup_www:
	@mkdir -p www/errors
	@if [ ! -f www/index.html ]; then \
		echo "<html><body><h1>Welcome to the server!</h1><p>It's working!</p></body></html>" > www/index.html; \
	fi
	@if [ ! -f www/test.html ]; then \
		echo "<html><body><h1>Test Page</h1><p>This is a test.</p></body></html>" > www/test.html; \
	fi
	@if [ ! -f www/errors/404.html ]; then \
		echo "<html><body><h1>404 - Page Not Found</h1><p>The requested resource does not exist.</p></body></html>" > www/errors/404.html; \
	fi
	@if [ ! -f www/errors/403.html ]; then \
		echo "<html><body><h1>403 - Forbidden</h1><p>You do not have permission to access this resource.</p></body></html>" > www/errors/403.html; \
	fi
	@if [ ! -f www/errors/500.html ]; then \
		echo "<html><body><h1>500 - Internal Server Error</h1><p>An error occurred on the server.</p></body></html>" > www/errors/500.html; \
	fi
	@if [ ! -f www/errors/503.html ]; then \
		echo "<html><body><h1>503 - Service Unavailable</h1><p>The server is overloaded. Please try again later.</p></body></html>" > www/errors/503.html; \
	fi
	@echo "www/ structure created with example pages"

# Run the server
run: $(TARGET)
	./$(TARGET)

# Basic tests (requires server running)
test:
	@echo "=== Basic Tests ==="
	@echo "Testing GET /index.html..."
	@curl -s http://localhost:8080/ > /dev/null && echo "✓ GET / OK" || echo "✗ GET / FAILED"
	@curl -s http://localhost:8080/test.html > /dev/null && echo "✓ GET /test.html OK" || echo "✗ GET /test.html FAILED"
	@echo "Testing HEAD /index.html..."
	@curl -s -I http://localhost:8080/ > /dev/null && echo "✓ HEAD / OK" || echo "✗ HEAD / FAILED"
	@echo "Testing 404..."
	@curl -s http://localhost:8080/naoexiste.html | grep -q 404 && echo "✓ 404 OK" || echo "✗ 404 FAILED"
	@echo "Testing HTTPS..."
	@curl -sk https://localhost:8443/ > /dev/null && echo "✓ HTTPS OK" || echo "✗ HTTPS FAILED"
	@echo "=== End of tests ==="

# Load test (requires apache bench)
loadtest:
	@echo "=== Load Test with Apache Bench ==="
	@which ab > /dev/null || (echo "Apache Bench not installed. Install with: sudo apt-get install apache2-utils" && exit 1)
	ab -n 1000 -c 50 http://localhost:8080/

# Check memory leaks with Valgrind
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Check race conditions with Helgrind
helgrind: $(TARGET)
	valgrind --tool=helgrind ./$(TARGET)

# Clean builds
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Objects and executable removed"

# Full cleanup (including orphaned IPC resources)
distclean: clean
	rm -f /dev/shm/webserver_shm_v1
	rm -f /dev/shm/sem.sem_ws_*
	rm -f access.log
	rm -rf www/
	@echo "IPC resources, logs and www/ removed"

help:
	@echo "Web Server IPC Project Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make all        - Compiles the server and creates www/ structure"
	@echo "  make run        - Compiles and runs the server"
	@echo "  make test       - Runs basic tests (requires running server)"
	@echo "  make loadtest   - Load test with Apache Bench"
	@echo "  make valgrind   - Checks memory leaks"
	@echo "  make helgrind   - Checks race conditions"
	@echo "  make clean      - Removes objects and executable"
	@echo "  make distclean  - Full cleanup (includes IPC)"
	@echo "  make help       - Shows this help"

.PHONY: all run test loadtest valgrind helgrind clean distclean help setup_www