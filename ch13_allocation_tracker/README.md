# Chapter 13 — Case Study: Allocation Tracker Agent

A complete JVMTI agent that samples object allocations and produces a JSON report.

## Features

- **SampledObjectAlloc event** ([JEP 331](https://openjdk.org/jeps/331), JDK 11+) — sees **all** allocation
  paths, including plain bytecode `new`, on a statistical sampling basis
- **Tunable sampling rate** via `SetHeapSamplingInterval` (64 KB here; the JVM default is 512 KB)
- **Per-class sample counts and byte totals** via a simple lookup table
- **Thread-safe** data access with a raw monitor
- **JSON output** (`allocations.json`) on JVM shutdown

> **Why not `VMObjectAlloc`?** It only fires for allocations the VM performs internally
> (JNI, reflection) — it misses every `new` in Java bytecode, which is nearly all allocation
> in a typical app. See the book's Chapter 13, §13.2.

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

(On Windows use `build\alloc_tracker.dll`, on macOS `build/liballoc_tracker.dylib`.)

## Expected Output

```
[Tracker] alloc_tracker loaded (Chapter 13 Case Study)
AllocTestApp: Starting allocation-heavy workload...
...
[Tracker] Sampled allocations for 12 classes. See allocations.json for details.
```

The `allocations.json` file contains a structured report of the sampled classes.

> **Reading sampled numbers:** These are *samples*, not totals — with a 64 KB interval each
> sample statistically represents ~64 KB of allocation. Exact numbers vary run to run; what's
> stable (and what matters) is the *ranking* of allocation-heavy classes.

## Requirements

- JDK 11 or newer (the agent fails fast with a clear message on older JVMs)
