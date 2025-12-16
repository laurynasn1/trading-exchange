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
BM_InsertOrder/0/iterations:1000000/manual_time              397 ns          507 ns      1000000
BM_InsertOrder/1/iterations:1000000/manual_time             1199 ns         1323 ns      1000000
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

# Optimization 2: Price-bucketed arrays instead of `std::map` and intrusive lists instead of `std::list`

`std::map` is node-based and chasing pointers is cache-unfriendly. Since the price is very likely to stay all the time in some range, e.g. from 0 to 10000, we can create price level for each price point. To avoid `double` we can convert the price to cents or ticks. Converting price to integers will also avoid floating-point price comparisons.

To achieve even better cache locality, we can use intrusive lists instead of `std::list`. Each order will have pointers to next and previous orders. The list will only have head and tail pointers.

Here are the bechmark results with all of the above optimizations applied:
```
Benchmark                                                      Time             CPU   Iterations
------------------------------------------------------------------------------------------------
BM_InsertOrder/0/iterations:1000000/manual_time             82.2 ns          150 ns      1000000
BM_InsertOrder/1/iterations:1000000/manual_time              334 ns          486 ns      1000000
BM_MatchSingle/manual_time                                   314 ns          685 ns      2655560
BM_MatchOrder/1/manual_time                                12122 ns        41930 ns        58709
BM_MatchOrder/10/manual_time                               11999 ns        41742 ns        51977
BM_MatchOrder/100/manual_time                              12356 ns        41760 ns        57136
BM_CancelOrder/1/iterations:1000000/manual_time              856 ns          864 ns      1000000
BM_CancelOrder/1000/iterations:1000000/manual_time           839 ns          847 ns      1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time        833 ns          841 ns      1000000
```

Order insertion at a fixed price point went from 397 ns to 82.2 ns (~4.8 times speedup), at different price points from 1199 ns to 334 ns (~3.6 times speedup).
Matching order that goes through 100 price levels went from 19911 ns to 12356 ns (~1.6 times speedup).
Order canceling at 1 and 1000 price levels slowed down a little bit but at 1000000 levels it sped up by ~1.8 times.

End-to-end throughput went from 774893 orders/sec to 1291155 orders/sec (~1.7 times speedup).
End-to-end latency is also better:
```
Min:           354 ns
Mean:          813 ns
Median:        718 ns
P95:          1453 ns
P99:          1817 ns
P99.9:        2574 ns
Max:         67281 ns
```

`perf stat` output:
```
    15,010,707,214      cycles                                                               (39.74%)
    15,178,679,539      instructions                     #    1.01  insn per cycle           (49.81%)
        75,982,568      cache-references                                                     (49.90%)
        27,382,192      cache-misses                     #   36.037 % of all cache refs      (50.09%)
     5,988,592,173      L1-dcache-loads                                                      (50.45%)
       155,387,370      L1-dcache-load-misses            #    2.59% of all L1-dcache accesses  (50.45%)
        56,215,562      LLC-loads                                                            (40.31%)
        20,727,432      LLC-load-misses                  #   36.87% of all LL-cache accesses  (10.03%)
     3,840,148,806      branches                                                             (19.96%)
        18,129,858      branch-misses                    #    0.47% of all branches          (29.81%)
               515      context-switches
            33,900      page-faults
```
Even though cache miss rate is somewhat the same, there were fewer branch misprediction and LLC load misses which increased the overall throughput.

Looking at `perf record` flamegraphs, there are still `std::unordered_map` and `std::shared_ptr` overheads which can be eliminated.

Commit [b46f7b2](https://github.com/laurynasn1/trading-exchange/commit/b46f7b2584f67afca490b37701cd308ab11391cd) denotes version after applying first two optimizations.

# Optimization 3: Vector instead of `std::unordered_map` for order cancelation and removal

If we know that order ids are in some defined range, we can use an array or `std::vector` instead of `std::unordered_map` to avoid hashing overhead.

Benchmark results:
```
Benchmark                                                      Time             CPU   Iterations
------------------------------------------------------------------------------------------------
BM_InsertOrder/0/iterations:1000000/manual_time             70.7 ns          138 ns      1000000
BM_InsertOrder/1/iterations:1000000/manual_time              226 ns          309 ns      1000000
BM_MatchSingle/manual_time                                   149 ns          439 ns      4734127
BM_MatchOrder/1/manual_time                                 7235 ns        24973 ns        96672
BM_MatchOrder/10/manual_time                                7331 ns        25276 ns        85413
BM_MatchOrder/100/manual_time                               7583 ns        25221 ns        90264
BM_CancelOrder/1/iterations:1000000/manual_time              370 ns          380 ns      1000000
BM_CancelOrder/1000/iterations:1000000/manual_time           369 ns          379 ns      1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time        374 ns          384 ns      1000000
```

Every benchmark improved, most notably matching and canceling benchmarks.

End-to-end throughput: 1705029 orders/sec.
End-to-end latency:
```
Min:           265 ns
Mean:          621 ns
Median:        547 ns
P95:           879 ns
P99:          1270 ns
P99.9:        7443 ns
Max:         56586 ns
```

Commit [ad6007e](https://github.com/laurynasn1/trading-exchange/commit/ad6007e5e236eb575b1566f13de2de32fd5e4fca) denotes version after applying all of the above optimizations.

# Optimization 4: Object pool and raw pointers

`std::shared_ptr` has a quite significant overhead. Instead of letting C++ cleanup no longer used objects, we can use raw pointers and manage the memory ourselves. We will need our custom allocator and deallocator and an object pool (or memory pool) is a great structure for this. We will pre-allocate empty orders in memory and use them for custom allocation and deallocation.

Results:
```
Benchmark                                                      Time             CPU   Iterations
------------------------------------------------------------------------------------------------
BM_InsertOrder/0/iterations:1000000/manual_time              126 ns          176 ns      1000000
BM_InsertOrder/1/iterations:1000000/manual_time              111 ns          157 ns      1000000
BM_MatchSingle/manual_time                                   109 ns          292 ns      6280596
BM_MatchOrder/1/manual_time                                 2628 ns        15697 ns       267544
BM_MatchOrder/10/manual_time                                2696 ns        16030 ns       260622
BM_MatchOrder/100/manual_time                               2945 ns        16186 ns       237653
BM_CancelOrder/1/iterations:1000000/manual_time              207 ns          216 ns      1000000
BM_CancelOrder/1000/iterations:1000000/manual_time           209 ns          218 ns      1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time        268 ns          277 ns      1000000
```

Throughput: 2781641 orders/sec
Latency:
```
Min:           259 ns
Mean:          446 ns
Median:        430 ns
P95:           571 ns
P99:           654 ns
P99.9:         793 ns
Max:         85148 ns
```

Commit [50d775f](https://github.com/laurynasn1/trading-exchange/commit/50d775f01090447c982277a6641eb37fcd3528ca) denotes version after applying all of the above optimizations.

# Optimization 5: Market data output policy instead of returning `std::vector`

Instead of allocating and returning `std::vector<MarketDataEvent>` we can emit market events in-place according to the output policy of matching engine.

Results:
```
Benchmark                                                      Time             CPU   Iterations
------------------------------------------------------------------------------------------------
BM_InsertOrder/0/iterations:1000000/manual_time             29.8 ns         80.4 ns      1000000
BM_InsertOrder/1/iterations:1000000/manual_time             28.9 ns         77.8 ns      1000000
BM_MatchSingle/iterations:10000000/manual_time              28.0 ns          116 ns     10000000
BM_MatchOrder/1/manual_time                                 1066 ns         4867 ns       598592
BM_MatchOrder/10/manual_time                                1070 ns         4991 ns       644399
BM_MatchOrder/100/manual_time                               1355 ns         5529 ns       499433
BM_CancelOrder/1/iterations:1000000/manual_time              197 ns          205 ns      1000000
BM_CancelOrder/1000/iterations:1000000/manual_time           196 ns          205 ns      1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time        256 ns          265 ns      1000000
```

Throughput: 4219409 orders/sec
Latency:
```
Min:           250 ns
Mean:          405 ns
Median:        387 ns
P95:           520 ns
P99:           602 ns
P99.9:         741 ns
Max:         49762 ns
```

Commit [69ea23e](https://github.com/laurynasn1/trading-exchange/commit/69ea23eaaf194577fc9a5ae1abf4208d886e6bfd) denotes version after applying all of the above optimizations.

# Small adjustments and final results

Added thread pinning to each thread to run on different processor cores. Rejection reason as `std::string` was replaced with RejectionType enum to avoid unnecessary allocation. There were some changes in order insertion benchmarks. Adding sequentially increasing prices is actually not the best benchmark as CPU predicts and prefetches next prices into cache thus making it very fast. Better benchmark is to add random price points, therefore instead of `BM_InsertOrder` benchmarks there will be `BM_InsertOrderFixed`, which inserts all orders at the same price, and `BM_InsertOrderRandom`, which inserts orders at random prices.

Final benchmark results:
```
Benchmark                                                      Time             CPU   Iterations
------------------------------------------------------------------------------------------------
BM_InsertOrderFixed/iterations:1000000/manual_time          24.2 ns         53.4 ns      1000000
BM_InsertOrderRandom/iterations:1000000/manual_time         90.8 ns          118 ns      1000000
BM_MatchSingle/iterations:10000000/manual_time              24.1 ns         69.6 ns     10000000
BM_MatchOrder/1/manual_time                                 1138 ns         3170 ns       624784
BM_MatchOrder/10/manual_time                                1158 ns         3231 ns       608336
BM_MatchOrder/100/manual_time                               1400 ns         3477 ns       502689
BM_CancelOrder/1/iterations:1000000/manual_time              196 ns          205 ns      1000000
BM_CancelOrder/1000/iterations:1000000/manual_time           198 ns          207 ns      1000000
BM_CancelOrder/1000000/iterations:1000000/manual_time        263 ns          273 ns      1000000
```

Final end-to-end tests results:
Throughput: 4310344 orders/sec
Latency:
```
Min:           182 ns
Mean:          334 ns
Median:        321 ns
P95:           449 ns
P99:           538 ns
P99.9:         643 ns
Max:         60069 ns
```

`perf stat` output:
```
     3,715,818,121      cycles                                                               (39.46%)
     2,750,415,162      instructions                     #    0.74  insn per cycle           (49.94%)
        43,759,966      cache-references                                                     (50.54%)
         6,858,021      cache-misses                     #   15.672 % of all cache refs      (51.13%)
     1,197,954,048      L1-dcache-loads                                                      (51.93%)
        72,573,188      L1-dcache-load-misses            #    6.06% of all L1-dcache accesses  (52.47%)
        28,280,075      LLC-loads                                                            (40.95%)
         2,729,238      LLC-load-misses                  #    9.65% of all LL-cache accesses  (9.63%)
       659,278,714      branches                                                             (20.00%)
        10,713,541      branch-misses                    #    1.63% of all branches          (29.58%)
               137      context-switches
            31,259      page-faults
```

Commit [cf46003](https://github.com/laurynasn1/trading-exchange/commit/cf46003de7b712bfb8602924d81881913a6e7e4a) denotes the final version after applying all optimizations.