# Low-Latency Limit Order Book in C++

## Tech stack

- Language: C++20
- Build: CMake
- Testing: Google Test, Google Benchmark

## Environment

- Intel(R) Core(TM) i7 CPU 920 @ 2.67GHz
- Debian GNU/Linux 12 (bookworm)
- gcc (Debian 12.2.0-14+deb12u1) 12.2.0

## Functionality

- Supports `SubmitOrder` and `CancelOrder` operations.
- Supports market, limit, IOC and FOK orders.
- Prices from $0.01 to $10000.00 (1 to 1000000 cents).
- 50 stock symbols.

## Architecture

- Price-bucketed arrays for O(1) price -> orders lookup.
- Array for O(1) orderId -> order lookup.
- Intrusive linked lists for better cache locality.
- Object pool for zero allocations on hot path.
- Lock-free queues between components (ring buffer).
- Compile-time polymorphism for zero-copy output.

## Optimization

My optimization journey can be found in [Optimization.md](https://github.com/laurynasn1/trading-exchange/blob/master/Optimization.md).

## Performance
| Metric | Value |
|--------|-------|
| Min Latency | 182ns |
| Mean Latency | 334ns |
| Median Latency | 321ns |
| P95 Latency | 449ns |
| P99 Latency | 538ns |
| P99.9 Latency | 643ns |
| Max Latency | 60µs |
| Throughput | 4.3M orders/sec |

## Testing scenarios

### Benchmarks

These benchmarks test only the matching engine module.

Order insertion benchmarks:

- `BM_InsertOrderFixed` tests order insertion at a fixed price point.
- `BM_InsertOrderRandom` tests order insertion at random price points.

Order matching benchmarks:

- `BM_MatchSingle` tests best case scenario of a matching order - when it is matched and filled with the first (top) order in the book.
- `BM_MatchOrder/N` tests matching order when there are N price levels. In each variation total number of resting orders is 100, so there are 100 / N orders per price level.

Order canceling benchmarks:

- `BM_CancelOrder/N` tests canceling order when there are N price levels. Similarly as in `BM_MatchOrder/N`, the total number of orders is 1'000'000 and there are 1'000'000 / N orders per price level. Orders are canceled in randomized permutation.

### End-to-end tests

End-to-end tests incorporate mock order gateway, matching engine and mock market data publisher.

Mock order gateway pre-generates various random operations with different probabilities:
- 40% limit orders that rest
- 30% limit orders that match
- 20% market orders
- 10% cancel orders

Mock market data publisher reads market data events and updates statistics: orders acked, orders filled, orders canceled and orders rejected.

There are 2 end-to-end tests: throughput and latency. Throughput test fires all orders at once and measures how long it took to process them all. Meanwhile latency test sends orders one by one, waiting for it to complete and records the latency.

## Build and Run

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
make -C build/ -j
./build/bench
./build/endtoend
```