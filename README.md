# mPSI with preprocessing

The implementation of our paper [Practical Multi-party Private Set Intersection with Reducible Zero-sharing](https://eprint.iacr.org/2025/2019), which is accepted by S&P 2026.

## Building the Project

Use the following commands.

```bash
cmake -S . -B out/build/linux -DCMAKE_BUILD_TYPE=Release -DENABLE_VOLEPSI=ON -DFETCH_VOLEPSI_IMPL=ON
cmake --build  out/builg/linux
```

## Running the code

We recommend using `run.sh` to run the code. Use the following instruction.

```bash
./run.sh -mpsi [need_ttp] [party_number] [corruption_threshold] [log2_size_of_set] [intersection_size]
```

For example, the following instruction runs a unit test where there are 15 parties with set of $2^{20}$, the corruption threshold is 13, and no ttp is needed for offline correlation generation. The size of the result intersection is 1000.

```bash
./run.sh -mpsi 0 15 13 20 1000
```

We recommend to run a benchmark using `perf.py`. It will run 10 times for each case and record the average.

```bash
python3 perf.py [Parameters]
```

Required parameters:

* `-n [value list]`: the list of number of parties, e.g., `-n 4,10,15`
* `-t [value list]`: the list of corruption threshold, e.g., `-t 1,3,7,14`
* `-m [value list]`: the list of log2 size of input set, e.g., `-m 12,16,20`
* `-r [file path]`: [Optional] use prior benchmark result. It is useful when you want to append new result to existing benchmark result or modify part of existing benchmark result
* `-o [file path]`: the path of benchmark result file

Some examples:

```bash
# first benchmark
python3 perf.py -n 4,10,15 -t 1,3,7,14 -m 12,16,20 -o test.csv

# add new cases (n,t,m)=(12,10,12/16/20)
python3 perf.py -n 12 -t 10 -m 12,16,20 -r test.csv -o test.csv

# re-benchmark case (n,t,m)=(4,1,12)
python3 perf.py -n 4 -t 1 -m 12 -r test.csv -o test.csv
```
