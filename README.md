Web Server Multi-Threaded com IPC e Semáforos
Servidor HTTP/1.1 multi-processo e multi-threaded implementado em C, usando memória partilhada POSIX e semáforos para sincronização entre processos.

📋 Características
✅ Arquitetura Master-Worker com múltiplos processos
✅ Thread pool em cada worker para concorrência
✅ Memória partilhada POSIX para comunicação entre processos
✅ Semáforos POSIX para sincronização
✅ Cache thread-safe com reader-writer locks
✅ Suporte para métodos GET e HEAD
✅ Servir ficheiros estáticos (HTML, CSS, JS, imagens)
✅ Deteção automática de MIME types
✅ Páginas de erro personalizadas (404, 403, 500, 503)
✅ Sistema de logging thread-safe
✅ Estatísticas em tempo real
✅ Configuração via ficheiro
🚀 Quick Start
Compilar
bash
make
Executar
bash
./server
O servidor iniciará na porta 8080 (configurável em server.conf).

Testar
Em outro terminal:

bash
# Testar no browser
firefox http://localhost:8080/

# Ou com curl
curl http://localhost:8080/

# Executar suite de testes
./test_server.sh
Parar o Servidor
Pressione CTRL+C no terminal onde o servidor está a correr. O servidor fará shutdown gracefully, limpando todos os recursos IPC.

📁 Estrutura do Projeto
.
├── main.c              # Ponto de entrada
├── master.c/h          # Processo master (gestão de workers)
├── worker.c/h          # Processos worker (thread pools)
├── http.c/h            # Handling de HTTP requests/responses
├── thread_pool.c/h     # Gestão de thread pools
├── cache.c/h           # Sistema de cache thread-safe
├── logger.c/h          # Sistema de logging
├── stats.c/h           # Estatísticas do servidor
├── config.c/h          # Parser de configuração
├── shared_mem.c/h      # Gestão de memória partilhada
├── semaphores.c/h      # Gestão de semáforos POSIX
├── Makefile            # Sistema de build
├── server.conf         # Ficheiro de configuração
├── test_server.sh      # Script de testes
└── www/                # Diretório root dos ficheiros web
    ├── index.html
    ├── test.html
    └── errors/
        ├── 404.html
        ├── 403.html
        ├── 500.html
        └── 503.html
⚙️ Configuração
Edite server.conf para alterar parâmetros:

ini
PORT=8080                    # Porta TCP
DOCUMENT_ROOT=www            # Diretório root
NUM_WORKERS=4                # Número de processos worker
THREADS_PER_WORKER=10        # Threads por worker
MAX_QUEUE_SIZE=100           # Tamanho da fila de conexões
LOG_FILE=access.log          # Ficheiro de log
CACHE_SIZE_MB=10             # Tamanho da cache (MB)
TIMEOUT_SECONDS=30           # Timeout
🏗️ Arquitetura
Hierarquia de Processos
Master Process
├── Accept conexões TCP
├── Gere workers
└── Monitoriza estatísticas
    │
    ├── Worker 1
    │   ├── Thread 1
    │   ├── Thread 2
    │   ├── ...
    │   └── Thread N
    │
    ├── Worker 2
    │   └── (mesma estrutura)
    │
    └── Worker N...
Mecanismos IPC
Memória Partilhada (POSIX shm)
Fila de conexões (circular buffer)
Estatísticas globais
Flags de controlo
Semáforos POSIX Nomeados
sem_empty: Slots vazios na fila
sem_full: Slots cheios na fila
sem_mutex: Exclusão mútua para acesso à fila
sem_stats: Proteção das estatísticas
sem_log: Proteção do ficheiro de log
Reader-Writer Locks (pthread_rwlock)
Cache de ficheiros thread-safe
Múltiplos leitores simultâneos
Escritores com acesso exclusivo
🧪 Testes
Testes Básicos
bash
make test
Teste de Carga
bash
# Requer apache2-utils
sudo apt-get install apache2-utils

# 1000 requests, 50 concorrentes
make loadtest
Verificar Memory Leaks
bash
make valgrind
Verificar Race Conditions
bash
make helgrind
🔧 Comandos Make
Comando	Descrição
make ou make all	Compila o servidor
make run	Compila e executa
make test	Testes básicos
make loadtest	Teste de carga
make valgrind	Verifica memory leaks
make helgrind	Verifica race conditions
make clean	Remove objetos e executável
make distclean	Limpeza completa (inclui IPC)
make help	Mostra ajuda
📊 Estatísticas
O servidor imprime estatísticas a cada 5 segundos e no shutdown:

=== Estatísticas ===
Pedidos Totais: 500
Bytes:          25600
Ativos:         4
====================
🐛 Debugging
Verificar Recursos IPC Órfãos
bash
# Memória partilhada
ls -l /dev/shm/webserver_*

# Remover manualmente se necessário
rm /dev/shm/webserver_shm_v1

# Semáforos
ls -l /dev/shm/sem.sem_ws_*
Monitorizar Conexões
bash
watch -n 1 'netstat -an | grep :8080'
Ver Árvore de Processos
bash
pstree -p $(pgrep -f ./server)
🔒 Segurança
Validação de paths (previne directory traversal)
Tratamento de buffer overflows
Verificação de permissões de ficheiros
Gestão segura de recursos
📝 Logs
O servidor gera logs no formato Apache Combined:

[2025-12-02 14:30:15] 127.0.0.1 "GET /index.html" 200 1234
[2025-12-02 14:30:16] 127.0.0.1 "GET /test.css" 200 567
⚠️ Limitações Conhecidas
Apenas métodos GET e HEAD
Sem suporte para HTTPS/TLS
Sem autenticação
Sem keep-alive (Connection: close)
Sem chunked transfer encoding
📚 Requisitos
SO: Linux (Ubuntu 20.04+ recomendado)
Compiler: GCC 9.0+
Libraries: pthread, rt (realtime)
Ferramentas: make, curl (para testes)
🤝 Contribuição
Trabalho académico para a disciplina de Sistemas Operativos 2025/2026.

📄 Licença
Projeto académico - ver guiões do projeto para detalhes.

