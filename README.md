<h1 align="center">
Graph Tool
<img src="https://img.shields.io/badge/language-C%2B%2B-%23f34b7d.svg" />
<img src="https://img.shields.io/badge/platform-Windows-blue" />
</h1>

## Preview
<img width="1920" height="1140" alt="image" src="https://github.com/user-attachments/assets/bc12fd83-4708-4ea2-bdd0-ffde6692b7dd" />
<img width="1920" height="1140" alt="image" src="https://github.com/user-attachments/assets/8532ddfc-5234-4c04-baab-cb4f802fdc63" />

## Features
A high-performance desktop app for visualizing and stepping through classic graph algorithms in real time, built with C++ and Qt.

- [x] **Interactive editing** - add nodes and edges directly.
- [x] **Step-by-step visualization** - watch algorithms execute node by node.
- [x] **Multiple algorithms supported** - a list of available algorithms is listed below.
- [x] **Weighted graph support**
- [x] **Oriented/unoriented graph support**
- [x] **Quadtree implementation** - performance improvement for larger scale graphs.
- [x] **Adaptive Storage Strategy** - automatically selects between adjacency list and adjacency matrix based on graph size and density to ensure optimal performance.
- [x] **PBF map loading** - import real-world road network data from OpenStreetMap `.pbf` files and run algorithms on actual map data
- [x] **JSON save / load** - export any graph to a `.graph` file and reload it later, making it easy to share graphs or pick up where you left off
- [x] **Dark/Light theme**

## Algorithms

| Algorithm | Type | Description |
|---|---|---|
| **GS** | Traversal | Generic Search |
| **BFS** | Traversal | Breadth-First Search |
| **DFS** | Traversal | Depth-First Search |
| **Path reconstruction** | Path | Path reconstruction based on traversal information (unweighted) |
| **Topological Sort** | Other | Yields the topological order of nodes |
| **Connected Components** | Other | Yields weakly connected components |
| **Strongly Connected Components** | Other | Yields strongly connected components (oriented) |
| **Generic MST** | Minimum Spanning Tree | Generic aproach MST |
| **Prim** | Minimum Spanning Tree | Node-based greedy MST |
| **Kruskal** | Minimum Spanning Tree | Edge-based greedy MST |
| **Boruvka** | Minimum Spanning Tree | Component-based greedy MST |
| **Dijkstra** | Shortest Path | Single-source shortest path (weighted) |
| **Floyd–Warshall** | Shortest Path | All pairs of shortest paths |
| **Floyd–Warshall (Path Reconstruction)** | Shortest Path | Path recovery between 2 nodes |

## Building
TODO

## Dependencies

- [libosmium](https://github.com/osmcode/libosmium) - PBF/OSM file parsing
- [simdjson](https://github.com/simdjson/simdjson) - high-performance JSON parsing
- [Qt](https://www.qt.io/download-qt-installer-oss) - used for the actual drawing

⚠️ Qt must be downloaded and installed manually.
