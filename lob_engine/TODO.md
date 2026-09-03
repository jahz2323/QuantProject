
Phase 1: Ingestion & Lock-Free C++ Engine

    Engineering: Write a C++ process using Boost.Asio or native sockets that consumes level-2 order book quotes. Interface with KX (kdb+) using its native C API (k.h) to read historical ticks or persist streaming depth states.

    Math & Finance to Learn: Limit Order Book (LOB) dynamics, price-time priority, bid-ask spread mechanics, and Order Flow Imbalance (OFI).

    Key Benchmark: Achieve zero heap-allocations during hot-path book updating.

[Exchange / Data Source] 
       │  (WebSocket / TCP via Boost.Asio)
       ▼
[C++ Feed Handler]
  ├── 1. Parse raw message (JSON / FIX / SBE)
  ├── 2. Update local C++ Limit Order Book
  └── 3. Package bid/ask into k.h data structure
       │  (Binary IPC push)
       ▼
[kdb+ Database]
  ├── Tickerplant (.u.upd)
  ├── In-Memory DB (Real-time queries)
  └── Historical DB (Persisted end-of-day disk storage)