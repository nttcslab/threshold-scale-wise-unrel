# Outage-Scale-Based Network Reliability Evaluation for Severe Reliability Requirements

This repository includes the codes to reproduce the results of experiments in the paper "Outage-Scale-Based Network Reliability Evaluation for Severe Reliability Requirements" accepted for IEEE GLOBECOM 2024.

## Requirements

We use [TdZdd](https://github.com/kunisura/TdZdd) for implementing the proposed method. Before building our code, you must place the header files of [TdZdd](https://github.com/kunisura/TdZdd) into this directory. More specifically, all header files of [TdZdd/include/tdzdd](https://github.com/kunisura/TdZdd/tree/master/include/tdzdd) must be placed on `tdzdd/` directory; e.g. `tdzdd/DdEval.hpp`, `tdzdd/DdSpec.hpp`, etc.

After that, if your environment has CMake version >=3.8, you can build all codes with the following commands:

```shell
(moving to src/ directory)
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

After running this, all binaries are generated in `release/` directory.

## How to reproduce experimental results

The built binary `main` implements our proposed method.

All data used in our experiments are in `data.tar.gz`. After extracting, `*.txt` describes the graph files (as an edgelist), and `*.txt.prob` specifies each link's availability. In addition, `*.txt.src` specifies the server node (the node having the highest betweenness centrality).

### Experiments

The proposed method can be executed by the following command:

```shell
./main [graph_file] [probability_file] [server_file] [order_file] [mode] [value] <client_file>
```

`[graph_file]`, `[probability_file]`, and `[server_file]` specify the path to the file describing the edgelist of the graph, each link's availability, and the list of server nodes, respectively. `[order_file]` specifies the path to the file describing the ordering among links. Note that we already computed the link order by the path-decomposition heuristics and wrote it on `*.txt`, so `[order_file]` can be specified with the same path as `[graph_file]` when reproducing the experimental results. `[mode]` switches what problem to solve: 
- If `[mode]=0`, the specific scale problem with $n^\prime=$`[value]` is solved.
- If `[mode]=1`, the scale threshold problem with $n^{\prime\prime}=$`[value]` is solved.
- If `[mode]=2`, the probability threshold problem with $p^\prime=$`[value]` is solved. 

`<client_file>` is an optional option that specifies the path to the file describing the list of client nodes. If it is not specified, all the nodes other than servers are set to clients.

After execution, the program computes the probability that more than `[value]` clients are disconnected from any servers when `[mode]=0`. when `[mode]=1` or `2`, the program computes the above probability from $n^\prime=1$ until threshold reaches.

_Running example:_ To solve probability threshold problem with $p^\prime=10^{-7}$ and IEEE 118-bus topology, run:

```shell
./main ../data/case118.m.edgelist.txt ../data/case118.m.edgelist.txt.prob ../data/case118.m.edgelist.txt.src ../data/case118.m.edgelist.txt 2 0.0000001
```

## License

This software is released under the NTT license, see `LICENSE.pdf`.
