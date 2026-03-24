# Chapter 13 — Case Study: Allocation Tracker Agent

A complete, production-quality JVMTI agent that tracks object allocations and produces a JSON report.

## Features

- **VMObjectAlloc callback** to track every JVM-visible allocation
- **Per-class allocation counts and byte totals** via a simple hash table
- **Thread-safe** data access with a raw monitor
- **JSON output** (`allocations.json`) on JVM shutdown

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac AllocTestApp.java
java -agentpath:./build/liballoc_tracker.so AllocTestApp
```

## Expected Output

```
[Tracker] alloc_tracker loaded (Chapter 13 Case Study)
AllocTestApp: Starting allocation-heavy workload...
...
[Tracker] Tracked 42 classes. See allocations.json for details.
```

The `allocations.json` file will contain a structured report of all tracked classes.
