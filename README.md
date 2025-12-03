
# Multi-Threaded Web Server with IPC and Semaphores

## Author 

Bernardo Mota Coelho 
125059

Tiago Francisco Crespo do Vale
125913

## Description

This project implements a multi-threaded HTTP server in C, using POSIX shared memory and semaphores for inter-process communication (IPC) and synchronization. It follows a master-worker architecture with worker processes handling requests using thread pools, allowing for efficient, concurrent request processing.

## Instalation

If repository is online and public: 

- type on terminal: git clone https://github.com/tiagofcvale/SO_Project2

- type on terminal: cd SO_Project2

If repository is in .rar or .zip:

- download the files, unzip and type on terminal: cd SO_Project2

## Usage 

- type on terminal: make

- type on terminal: ./server


To stop the server, press CTRL+C in the terminal where the server is running. The server will shut down gracefully, cleaning up all IPC resources.


## Features

 - Multi-process and Multi-threaded Architecture: A master process that manages worker      processes, with each worker running a thread pool to handle incoming HTTP requests concurrently.

- Inter-Process Communication (IPC): Shared memory is used for communication between processes, while semaphores ensure synchronization and avoid race conditions.

- HTTP/1.1 Protocol Support: Supports GET and HEAD requests, static file serving (HTML, CSS, JS, images), and custom error pages (404, 403, 500, 503).

- Thread-Safe Operations: Implemented using mutexes and condition variables for thread synchronization, ensuring thread safety for resources such as logs, statistics, and file cache.

- Logging System: A thread-safe logging mechanism that records every HTTP request and error.

- Statistics: Tracks the number of requests served, bytes transferred, and response times.

- Cache Management: A thread-safe LRU (Least Recently Used) cache to store frequently accessed files.

- Graceful Shutdown: Uses signal handling to ensure that resources are cleaned up properly when the server shuts down.

Optional or advanced features:

## Configuration 

The server starts on port 8080 (configurable in server.conf).

## Examples 



## References 

* [W3Schools](https://www.w3schools.com/)
* [Stack Overflow](https://stackoverflow.com/questions)
* [Modern Operating Systems 4th Edition - Tanenbaum & Bos] https://csc-knu.github.io/sys-prog/books/Andrew%20S.%20Tanenbaum%20-%20Modern%20Operating%20Systems.pdf

