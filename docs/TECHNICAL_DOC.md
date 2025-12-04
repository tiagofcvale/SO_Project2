## Author 
Bernardo Mota Coelho 
125059

Tiago Francisco Crespo do Vale
125913

## ========= System Architecture Diagram =======

![System Architecture Diagram](/diagrams/architecture_diagram.png)


## ============ Data Flow Diagrams ================

### **Cache Data Flow**  
![Cache Data Flow](/diagrams/cache_fchart.png)

---

### **Config Data Flow**  
![Config Data Flow](/diagrams/config_fchart.png)

---


### **HTTP Data Flow**  
![Http Data Flow](/diagrams/http_fchart.png)

---

### **Main Data Flow**  
![Main Data Flow](/diagrams/main_fchart.png)

---

### **Master Data Flow**  
![Master Data Flow](/diagrams/master_fchart.png)    

---

### **Semaphores Data Flow**  
![Semaphores Data Flow](/diagrams/semaphores_fchart.png)

---

### **SharedMem Data Flow**  
![SharedMem Data Flow](/diagrams/sharedMem_fchart.png)

---

### **Stats Data Flow**  
![Stats Data Flow](/diagrams/stats_fchart.png)

---

### **ThreadPool Data Flow**  
![ThreadPool Data Flow](/diagrams/threadPool_fchart.png)

---

### **Worker Data Flow**  
![Worker Data Flow](/diagrams/worker_fchart.png)

---

## ======== File descriptions =============

### `cache.c`
- **Description**: Implements an in-memory file cache for the HTTP server, using a hash table and reader-writer locks for thread-safe access. Provides functions to initialize the cache, insert and retrieve files, and clean up memory. Optimizes file serving by storing file contents and metadata, allowing concurrent reads and exclusive writes.

### `config.c`
- **Description**: Handles server configuration management. Loads settings from a configuration file, applies defaults if missing, and provides accessors for all configuration parameters (port, document root, workers, threads, queue size, log file, cache size, timeout). Includes string trimming and parsing utilities for robust config file handling.

### `global.c`
- **Description**: Defines global variables and shared state used across the server, such as flags, counters, and configuration pointers. Facilitates inter-module communication and centralizes global data management.

### `http.c`
- **Description**: Handles HTTP protocol logic, including parsing requests, building responses, managing headers, and serving files. Implements error handling and supports static file delivery from the document root.

### `logger.c`
- **Description**: Manages server logging, including access logs and error logs. Provides thread-safe functions to write log entries, format messages, and handle log file rotation or flushing.

### `main.c`
- **Description**: Entry point for the server application. Initializes all subsystems, parses command-line arguments, loads configuration, and starts the master process and worker threads. Coordinates server startup and shutdown.

### `master.c`
- **Description**: Implements the master process logic, responsible for spawning and managing worker processes, handling signals, and coordinating inter-process communication. Oversees server lifecycle and resource management.

### `semaphores.c`
- **Description**: Provides semaphore utilities for process and thread synchronization. Implements creation, destruction, and operations on POSIX semaphores to coordinate access to shared resources.

### `shared_mem.c`
- **Description**: Manages shared memory segments for inter-process communication. Handles allocation, mapping, and cleanup of shared memory used for statistics, configuration, or other shared data.

### `stats.c`
- **Description**: Collects and manages server statistics, such as request counts, errors, and performance metrics. Provides functions to update, retrieve, and reset statistics, supporting monitoring and reporting.

### `thread_pool.c`
- **Description**: Implements a thread pool for efficient request handling. Manages worker threads, task queues, and synchronization primitives to process incoming connections concurrently.

### `worker.c`
- **Description**: Contains the logic for worker processes/threads. Handles accepting connections, processing requests, interacting with the thread pool, and communicating with the master process. Executes the main request-processing loop.