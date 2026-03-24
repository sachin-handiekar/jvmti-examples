# Chapter 05 — Heap and Stack Analysis Agent

Provides heap and stack inspection capabilities:

- **Heap walking** with `IterateOverReachableObjects` to count and measure objects
- **Thread stack dumps** with `GetAllThreads` + `GetStackTrace`
- **Object sizing** with `GetObjectSize`
- Analysis at both VMInit and VMDeath

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac HeapStackTestApp.java
java -agentpath:./build/libheap_stack_agent.so HeapStackTestApp
```

## Expected Output

Check `heap_stack_agent.log` for detailed heap summary and thread stack traces.
