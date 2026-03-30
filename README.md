# C HTTP/HTTPS Client

A lightweight, dependency-minimal HTTP/1.1 & HTTPS client written in C99.

## Features
* **TLS Encryption:** Secure communication via OpenSSL.
* **Transfer-Encoding:** Full support for `chunked` data stream parsing.
* **Memory Safety:** Custom `ByteBuffer` management and `HeapList` for memory tracking.
* **Robustness:** Socket timeout protection (`SO_RCVTIMEO`) to prevent deadlocks.
* **Dynamic Parsing:** Custom URL parser for Scheme, Host, Port, and Path extraction.

## Requirements
* GCC / Clang
* CMake
* OpenSSL Library (`libssl-dev`)

## Build
```bash
mkdir build && cd build
cmake ..
make