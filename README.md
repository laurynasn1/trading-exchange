# Low-Latency Limit Order Book in C++

Simple limit order book in naive implementation: `std::map`, `std::unordered_map`, `std::shared_ptr` and etc. Will be optimized to achieve lower latency and higher throughput.

## Tech stack

- Language: C++17
- Build: CMake
- Testing: Google Test, Google Benchmark

## Functionality

- Supports market, limit, IOC and FOK orders.
- Supports `SubmitOrder` and `CancelOrder` operations.

## Testing scenarios

### Benchmarks

These benchmarks test only the matching engine module.

Order insertion benchmarks:

- `BM_InsertOrder/0` tests order insertion at a fixed price point.
- `BM_InsertOrder/1` tests order insertion at an increasing price point.

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