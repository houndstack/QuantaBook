# QuantaBook

QuantaBook is a C++20 market microstructure project focused on building a technically defensible exchange matching foundation first, then expanding into simulation and strategy research.

## Why This Project
Matching engines encode the mechanics behind price formation, queue priority, and fill outcomes. Building one from first principles is a practical way to demonstrate:
- Exchange rule correctness
- Deterministic systems design
- Performance-aware C++ engineering
- Microstructure intuition grounded in implementation details

## Current Scope (Phase 1 Only)
- Limit and market order submission
- Price-time priority matching
- Partial fills and multi-level sweeps
- Cancellation by order ID
- Best bid / best ask queries
- Deterministic trade generation
- Unit tests as executable behavior specification

## Planned Future Stages
- Event-driven exchange simulation with market regimes
- Strategy agents (inventory-aware and volatility-aware market making)
- Risk controls and PnL accounting
- Python analytics for reproducible experiment comparison

## Build
```bash
cmake -S . -B build
cmake --build build
```

## Test
```bash
ctest --test-dir build --output-on-failure
```

## Demo
```bash
./build/quantabook_demo
```

## Benchmark Scaffold
```bash
./build/benchmark_matching_engine
```

## Important Note
This project is a simulated exchange/research framework, not a claim of profitable real-world trading performance.
