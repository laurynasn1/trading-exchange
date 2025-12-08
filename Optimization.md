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
BM_InsertOrder/0/iterations:1000000/manual_time             60.5 ns          133 ns      1000000
BM_InsertOrder/1/iterations:1000000/manual_time              707 ns          843 ns      1000000
BM_MatchSingle/manual_time                                   360 ns          775 ns      2261845
BM_MatchOrder/1/manual_time                                12336 ns        44501 ns        55875
BM_MatchOrder/10/manual_time                               13363 ns        48833 ns        52346
BM_MatchOrder/100/manual_time                              19911 ns        61949 ns        35778
BM_CancelOrder/1/iterations:1000000/manual_time              542 ns          551 ns      1000000
BM_CancelOrder/1000/iterations:1000000/manual_time           669 ns          678 ns      1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time       1501 ns         1508 ns      1000000
```

End-to-end latency:
```
Min:           423 ns
Mean:         1108 ns
Median:       1090 ns
P95:          1655 ns
P99:          2041 ns
P99.9:        2783 ns
Max:         54120 ns
```

End-to-end throughput: 2000000 requests in 2989 ms (669120 orders/sec).

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
Min:           387 ns
Mean:         1062 ns
Median:       1059 ns
P95:          1583 ns
P99:          1939 ns
P99.9:        2675 ns
Max:         57599 ns

Throughput: 774893 orders/sec
```

Commit [a3f7f12](https://github.com/laurynasn1/trading-exchange/commit/a3f7f12deaaf0ee910a014e31ea037a240513888) denotes version after applying first optimization.