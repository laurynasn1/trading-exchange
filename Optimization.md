# Baseline

Baseline implementation includes mock order gateway, simple matching engine and mock market data publisher. Order book uses:
- `std::shared_ptr<Order>` for automatic memory management
- `std::map<double, std::list<std::shared_ptr<Order>>>` to track bid and ask orders
- `std::unordered_map<uint64_t, std::shared_ptr<Order>>` to map order id to order object for fast cancelation.

Matching engine has a map `std::unordered_map<std::string, std::unique_ptr<OrderBook>>` to lookup order books by stock symbol.

Modules communicate with each other via lock-free single-producer single-consumer queue.

Commit [8f81f92](https://github.com/laurynasn1/trading-exchange/commit/8f81f92346115dfd97699a355bd00f85f6977d5b) denotes the baseline version.

Benchmark results:

Benchmark                                              |       Time     |       CPU | Iterations
-------------------------------------------------------|----------------|-----------|-----------
BM_InsertOrder/0/iterations:1000000/manual_time        |    66.2 ns     |    144 ns |    1000000
BM_InsertOrder/1/iterations:1000000/manual_time        |     915 ns     |   1034 ns |    1000000
BM_MatchSingle/manual_time                             |     237 ns     |    574 ns |    2900058
BM_MatchOrder/1/manual_time                            |   12698 ns     |  52832 ns |      55914
BM_MatchOrder/10/manual_time                           |   13534 ns     |  54510 ns |      51397
BM_MatchOrder/100/manual_time                          |   20394 ns     |  68628 ns |      34378
BM_CancelOrder/1/iterations:1000000/manual_time        |    40.0 ns     |   49.2 ns |    1000000
BM_CancelOrder/1000/iterations:1000000/manual_time     |    40.3 ns     |   49.5 ns |    1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time  |    40.2 ns     |   49.5 ns |    1000000

End-to-end throughput: 2000000 requests in 2469 ms (810045 orders/sec).

End-to-end latency:
- Min:           267 ns
- Mean:         1002 ns
- Median:       1025 ns
- P95:          1496 ns
- P99:          1843 ns
- P99.9:        3006 ns
- Max:         66988 ns
