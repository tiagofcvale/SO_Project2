CC = gcc
CFLAGS = -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -g -Isrc
LDFLAGS = -pthread -lrt

# Diretórios
SRC_DIR = src
BUILD_DIR = build

TARGET = server

# Ficheiros fonte na pasta src/
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/master.c $(SRC_DIR)/worker.c $(SRC_DIR)/http.c \
       $(SRC_DIR)/thread_pool.c $(SRC_DIR)/cache.c $(SRC_DIR)/logger.c $(SRC_DIR)/stats.c \
       $(SRC_DIR)/config.c $(SRC_DIR)/shared_mem.c $(SRC_DIR)/semaphores.c $(SRC_DIR)/global.c

# Objetos na pasta build/
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all run test loadtest valgrind helgrind clean distclean help setup_www

# Regra padrão
all: $(BUILD_DIR) $(TARGET) setup_www

# Criar diretório build se não existir
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compilar o executável
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build completo: $(TARGET)"

# Compilar cada arquivo .o a partir de .c
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependências explícitas
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/config.h $(SRC_DIR)/logger.h $(SRC_DIR)/stats.h $(SRC_DIR)/cache.h $(SRC_DIR)/master.h
$(BUILD_DIR)/master.o: $(SRC_DIR)/master.c $(SRC_DIR)/master.h $(SRC_DIR)/config.h $(SRC_DIR)/logger.h $(SRC_DIR)/worker.h $(SRC_DIR)/shared_mem.h $(SRC_DIR)/semaphores.h
$(BUILD_DIR)/worker.o: $(SRC_DIR)/worker.c $(SRC_DIR)/worker.h $(SRC_DIR)/config.h $(SRC_DIR)/thread_pool.h $(SRC_DIR)/shared_mem.h $(SRC_DIR)/semaphores.h
$(BUILD_DIR)/http.o: $(SRC_DIR)/http.c $(SRC_DIR)/http.h $(SRC_DIR)/config.h $(SRC_DIR)/logger.h $(SRC_DIR)/cache.h $(SRC_DIR)/stats.h $(SRC_DIR)/shared_mem.h $(SRC_DIR)/semaphores.h
$(BUILD_DIR)/thread_pool.o: $(SRC_DIR)/thread_pool.c $(SRC_DIR)/thread_pool.h $(SRC_DIR)/http.h
$(BUILD_DIR)/cache.o: $(SRC_DIR)/cache.c $(SRC_DIR)/cache.h
$(BUILD_DIR)/logger.o: $(SRC_DIR)/logger.c $(SRC_DIR)/logger.h $(SRC_DIR)/config.h
$(BUILD_DIR)/stats.o: $(SRC_DIR)/stats.c $(SRC_DIR)/stats.h
$(BUILD_DIR)/config.o: $(SRC_DIR)/config.c $(SRC_DIR)/config.h
$(BUILD_DIR)/shared_mem.o: $(SRC_DIR)/shared_mem.c $(SRC_DIR)/shared_mem.h $(SRC_DIR)/connection_queue.h $(SRC_DIR)/stats.h
$(BUILD_DIR)/semaphores.o: $(SRC_DIR)/semaphores.c $(SRC_DIR)/semaphores.h
$(BUILD_DIR)/global.o: $(SRC_DIR)/global.c $(SRC_DIR)/global.h

# Criar estrutura de diretórios e páginas de exemplo
setup_www:
	@mkdir -p www/errors
	@if [ ! -f www/index.html ]; then \
		echo "<html><body><h1>Bem-vindo ao servidor!</h1><p>Está a funcionar!</p></body></html>" > www/index.html; \
	fi
	@if [ ! -f www/test.html ]; then \
		echo "<html><body><h1>Página de teste</h1><p>Este é um teste.</p></body></html>" > www/test.html; \
	fi
	@if [ ! -f www/errors/404.html ]; then \
		echo "<html><body><h1>404 - Página não encontrada</h1><p>O recurso solicitado não existe.</p></body></html>" > www/errors/404.html; \
	fi
	@if [ ! -f www/errors/403.html ]; then \
		echo "<html><body><h1>403 - Acesso Proibido</h1><p>Não tem permissões para aceder a este recurso.</p></body></html>" > www/errors/403.html; \
	fi
	@if [ ! -f www/errors/500.html ]; then \
		echo "<html><body><h1>500 - Erro Interno do Servidor</h1><p>Ocorreu um erro no servidor.</p></body></html>" > www/errors/500.html; \
	fi
	@if [ ! -f www/errors/503.html ]; then \
		echo "<html><body><h1>503 - Serviço Indisponível</h1><p>O servidor está sobrecarregado. Tente novamente mais tarde.</p></body></html>" > www/errors/503.html; \
	fi
	@echo "Estrutura www/ criada com páginas de exemplo"

# Executar o servidor
run: $(TARGET)
	./$(TARGET)

# Testes básicos (requer servidor a correr)
test:
	@echo "=== Testes Básicos ==="
	@echo "A testar GET /index.html..."
	@curl -s http://localhost:8080/ > /dev/null && echo "✓ GET / OK" || echo "✗ GET / FALHOU"
	@curl -s http://localhost:8080/test.html > /dev/null && echo "✓ GET /test.html OK" || echo "✗ GET /test.html FALHOU"
	@echo "A testar HEAD /index.html..."
	@curl -s -I http://localhost:8080/ > /dev/null && echo "✓ HEAD / OK" || echo "✗ HEAD / FALHOU"
	@echo "A testar 404..."
	@curl -s http://localhost:8080/naoexiste.html | grep -q 404 && echo "✓ 404 OK" || echo "✗ 404 FALHOU"
	@echo "=== Fim dos testes ==="

# Teste de carga (requer apache bench)
loadtest:
	@echo "=== Teste de Carga com Apache Bench ==="
	@which ab > /dev/null || (echo "Apache Bench não instalado. Instale com: sudo apt-get install apache2-utils" && exit 1)
	ab -n 1000 -c 50 http://localhost:8080/

# Verificar memory leaks com Valgrind
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Verificar race conditions com Helgrind
helgrind: $(TARGET)
	valgrind --tool=helgrind ./$(TARGET)

# Limpar builds
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Objetos e executável removidos"

# Limpeza completa (incluindo recursos IPC órfãos)
distclean: clean
	rm -f /dev/shm/webserver_shm_v1
	rm -f /dev/shm/sem.sem_ws_*
	rm -f access.log
	rm -rf www/
	@echo "Recursos IPC, logs e www/ removidos"

# Ajuda
help:
	@echo "Makefile do Web Server IPC Project"
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  make all        - Compila o servidor e cria estrutura www/"
	@echo "  make run        - Compila e executa o servidor"
	@echo "  make test       - Executa testes básicos (requer servidor ativo)"
	@echo "  make loadtest   - Teste de carga com Apache Bench"
	@echo "  make valgrind   - Verifica memory leaks"
	@echo "  make helgrind   - Verifica race conditions"
	@echo "  make clean      - Remove objetos e executável"
	@echo "  make distclean  - Limpeza completa (inclui IPC)"
	@echo "  make help       - Mostra esta ajuda"

.PHONY: all run test loadtest valgrind helgrind clean distclean help setup_www