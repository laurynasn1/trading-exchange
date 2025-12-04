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

Order insertion benchmarks:

- `BM_InsertOrder/0` tests order insertion at a fixed price point.
- `BM_InsertOrder/1` tests order insertion at an increasing price point.

Order matching benchmarks:

- `BM_MatchSingle` tests best case scenario of a matching order - when it is matched and filled with the first (top) order in the book.
- `BM_MatchOrder/N` tests matching order when there are N price levels. In each variation total number of resting orders is 10'000, so there are 10'000 / N orders per price level.

Order canceling benchmarks:

- `BM_CancelOrder/N` tests canceling order when there are N price levels. Similarly as in `BM_MatchOrder/N`, the total number of orders is 1'000'000 and there are 1'000'000 / N orders per price level. Orders are canceled in randomized permutation.