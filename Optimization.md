# Baseline

Baseline implementation includes mock order gateway, simple matching engine and mock market data publisher. Order book uses:
- `std::shared_ptr<Order>` for automatic memory management
- `std::map<double, std::list<std::shared_ptr<Order>>>` to track bid and ask orders
- `std::unordered_map<uint64_t, std::shared_ptr<Order>>` to map order id to order object for fast cancelation.

Matching engine has a map `std::unordered_map<std::string, std::unique_ptr<OrderBook>>` to lookup order books by stock symbol.

Modules communicate with each other via lock-free single-producer single-consumer queue.

Benchmark results:
```
Benchmark                                                      Time             CPU   Iterations
------------------------------------------------------------------------------------------------
BM_InsertOrder/0/iterations:1000000/manual_time             66.2 ns          144 ns      1000000
BM_InsertOrder/1/iterations:1000000/manual_time              915 ns         1034 ns      1000000
BM_MatchSingle/manual_time                                   237 ns          574 ns      2900058
BM_MatchOrder/1/manual_time                                12698 ns        52832 ns        55914
BM_MatchOrder/10/manual_time                               13534 ns        54510 ns        51397
BM_MatchOrder/100/manual_time                              20394 ns        68628 ns        34378
BM_CancelOrder/1/iterations:1000000/manual_time             40.0 ns         49.2 ns      1000000
BM_CancelOrder/1000/iterations:1000000/manual_time          40.3 ns         49.5 ns      1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time       40.2 ns         49.5 ns      1000000
```

End-to-end latency:
```
Min:           267 ns
Mean:         1002 ns
Median:       1025 ns
P95:          1496 ns
P99:          1843 ns
P99.9:        3006 ns
Max:         66988 ns
```

End-to-end throughput: 2000000 requests in 2469 ms (810045 orders/sec).

`perf stat` output for throughput test:
```
22,435,789,154      cycles                                                               (39.92%)
21,440,196,699      instructions                     #    0.96  insn per cycle           (49.90%)
   145,897,230      cache-references                                                     (49.96%)
    48,173,978      cache-misses                     #   33.019 % of all cache refs      (50.01%)
 8,746,378,647      L1-dcache-loads                                                      (50.06%)
   194,972,836      L1-dcache-load-misses            #    2.23% of all L1-dcache accesses  (50.28%)
    76,582,313      LLC-loads                                                            (40.16%)
    23,979,741      LLC-load-misses                  #   31.31% of all LL-cache accesses  (10.03%)
 5,304,251,269      branches                                                             (20.04%)
    50,186,586      branch-misses                    #    0.95% of all branches          (30.00%)
           767      context-switches
        71,714      page-faults
```

Commit [8f81f92](https://github.com/laurynasn1/trading-exchange/commit/8f81f92346115dfd97699a355bd00f85f6977d5b) denotes the baseline version.

# Optimization 1: Pre-allocating order books with known stock symbols

Since the list of stock symbols is known before trading, we can pre-allocate an order book for each symbol. This way we will avoid allocating an order book (which is very costly) on the hot path. Also, since the number of symbols is known, we can use integers to map to the symbols thus avoiding unnecessary `std::string` operations.

This optimization improved end-to-end tests by a little bit:
```
Min:           295 ns
Mean:          975 ns
Median:        995 ns
P95:          1447 ns
P99:          1775 ns
P99.9:        2626 ns
Max:         65224 ns

Throughput: 922509 orders/sec
```

Commit [a3f7f12](https://github.com/laurynasn1/trading-exchange/commit/a3f7f12deaaf0ee910a014e31ea037a240513888) denotes version after applying first optimization.