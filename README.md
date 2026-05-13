# 🧭 Custom DNS Resolver

An asynchronous DNS resolver written in C++ with filtering, caching, and epoll support. Built as a learning project to dive deep into Linux system programming, binary protocol handling, and UDP sockets.

## ✨ Features

- **Asynchronous I/O** via epoll (Edge-Triggered)
- **Recursive resolution** through a public DNS server (1.1.1.1)
- **A-record caching** with TTL aware expiration
- **Domain filtering** – block ads/trackers via a simple text file
- **Server mode** – answers any DNS query, compatible with `dig`, browsers, and system resolvers
- **Binary DNS protocol support**, including name compression (`0xC0`)
- **Strict C++**: `std::memcpy` instead of strict aliasing violations, RAII for sockets

## 🧱 Architecture (current state)

At this stage most of the logic resides in `main.cpp` and the `Resolver`, `parser`, and `response` modules. This is a prototype where business logic, networking, and dispatching are still tightly coupled. One of the highest priority tasks is extracting abstraction layers (server, cache, filter) to improve readability and maintainability.

## 📁 Project structure
```text
dns-resolver/
├── CMakeLists.txt
├── config/
│ └── blocked_domains.txt # List of domains to block
├── src/
│ ├── main.cpp # Main loop, epoll, dispatching
│ ├── network/
│ │ └── udp_socket.h/cpp # UDP socket creation and setup
│ ├── dns/
│ │ ├── resolver.h/cpp # Sending queries to 1.1.1.1, pending storage, caching
│ │ ├── parser.h/cpp # DNS packet parsing (header, question, compression)
│ │ └── response.h/cpp # Building responses for clients
│ ├── utils/
│ │ └── parser.h/cpp # Helper functions (parse_dns_name)
│ └── entities/
│ └── dns.h # Data structures (DnsHeader, PendingQuery, etc.)
└── README.md
```


## ⚙️ Build & Run

**Requirements:** CMake ≥ 3.16, C++20 compiler, Linux.

```bash
git clone <your-repo-url>
cd dns-resolver
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```
# Run (needs privileges for port 53, or use 5353)
./dns_resolver

By default the server listens on port 5353 to avoid needing root privileges. You can change the port inside src/main.cpp (constant DNS_PORT).
🛡️ Filtering setup

The file config/blocked_domains.txt contains one domain per line. Example:
text
```text
doubleclick.net
ads.google.com
googletagmanager.com
```
On startup the server loads this list and responds with 0.0.0.0 to any blocked domain. You can easily extend the list with new domains.
# 🌐 Using as system resolver

To route your system's DNS queries through this server:
```bash
# Temporary (until reboot)
sudo resolvectl dns <interface> 127.0.0.1#5353

# Test
dig @127.0.0.1 -p 5353 example.com
```
After this all programs (including browsers) will use your resolver. You'll immediately notice ads disappearing from blocked domains, and cache acceleration for repeated queries.
# 🧪 Usage examples
```bash
# Normal query
dig @127.0.0.1 -p 5353 google.com
# → 142.250.185.46 (real IP)

# Blocked domain
dig @127.0.0.1 -p 5353 doubleclick.net
# → 0.0.0.0

# Repeated query (cache hit)
dig @127.0.0.1 -p 5353 google.com
# → instant response
```
# 🚧 Known issues
- Technical debt: The code needs refactoring – extracting Server, Cache, Filter classes to separate concerns and simplify maintenance.
- No unit tests: Critical to cover parser, response builder, and caching with tests.
- Logging: Currently uses std::cout, planned migration to spdlog.
- Concurrency: In the future a one-loop-per-thread model could utilize all CPU cores.
- AAAA, CNAME support: Currently the resolver only handles A records (IPv4).
