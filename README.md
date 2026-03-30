# C HTTP/HTTPS Network Client

A high-performance, minimalist HTTP/1.1 & HTTPS client engine written in C99. Designed for reliability, memory efficiency, and strict adherence to the Single Responsibility Principle (SRP).

## 🏗 System Architecture & File Responsibilities

The monolithic architecture has been entirely refactored into strictly isolated layers. The orchestrator (`main.c`) is unaware of socket states, TLS handshakes, or protocol parsing logic.

### 1. Core Engine
* **`main.c`** (Orchestrator): The entry point. It delegates CLI parsing, network fetching, and file I/O to the respective subsystems in under 40 lines of code.
* **`http_client.c / .h`** (Protocol Layer): Manages the HTTP state machine. Dynamically constructs RFC 7230 compliant requests (GET, POST, PUT, DELETE) by calculating exact payload sizes (`Content-Length`) to prevent buffer overflows. Routes the raw network stream to the parsing layer.
* **`network.c / .h`** (Transport Layer): Abstracted socket management. Transparently handles both raw TCP (Port 80) and TLS-encrypted (Port 443) streams. Acts as a polymorphic router for `SendData` and `ReceiveData` based on the context state.

### 2. Protocol Parsing & Utility
* **`http_parser.c / .h`** (Data Extraction): Identifies the end of HTTP headers (`\r\n\r\n`) and safely extracts the payload using either the `Content-Length` header or real-time `Transfer-Encoding: chunked` hexadecimal decoding.
* **`url_parser.c / .h`** (Routing): Robust string manipulation to decompose complex URLs into Scheme, Host, Port, Path, and Method components. Handles optional CLI payload arguments for POST/PUT requests.
* **`bytebuffer.c / .h`** (Memory Management): A dynamic, auto-resizing data container (O(1) amortized append). Completely eliminates the need for arbitrary fixed-size arrays and prevents memory leaks during large network reads.

## 🛠 Technical Features

* **Dynamic Payload Delivery:** Full support for GET, POST, PUT, and DELETE methods.
* **Secure Communication:** TLS/SSL integration via OpenSSL with Server Name Indication (SNI) support.
* **Robustness:** Kernel-level socket timeouts (`SO_RCVTIMEO`) ensure the client never deadlocks on unresponsive servers.
* **Zero Memory Leaks:** Strict pointer lifecycle management; resources are freed exactly within the layer that allocated them.

## 🚀 Getting Started

### Prerequisites
* OpenSSL Development Headers (`libssl-dev` on Ubuntu/Debian)
* CMake 3.10+
* C99 Compatible Compiler (GCC/Clang)

### Build
```bash
mkdir build && cd build
cmake ..
make
```
### Usage
**1. Standard GET Request:**
```bash
./http_client https://www.example.com/
```

**2. POST Request with Payload:**
```bash
./http_client POST https://www.example.com/ "user=admin&pass=123"
```

*Note: The response body is strictly isolated from the HTTP headers and saved securely to `output_file.dat` in the execution directory.*

## 📜 License
Distributed under the **MIT License**.