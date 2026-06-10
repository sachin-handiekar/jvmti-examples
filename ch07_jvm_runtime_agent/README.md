# Chapter 07 — JVM Runtime Interaction Agent

Demonstrates JVM runtime inspection and GC monitoring:

- **GC events**: `GarbageCollectionStart` / `GarbageCollectionFinish` timed with
  `GetTime` (nanosecond wall-clock). GC callbacks run in a restricted environment —
  no JNI, no file I/O — so they do bookkeeping only; the summary is printed at `VMDeath`
- **System properties**: Reading `java.vm.name`, `java.vm.version`, etc.
- **Runtime queries**: `GetAvailableProcessors`, `GetPhase`
- **Object sizing**: Using `GetObjectSize` to measure object memory footprint

> Run with a small heap (e.g., `java -Xmx16m ...`) to force collections and see
> GC statistics in the summary.

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac RuntimeTestApp.java
java -agentpath:./build/libjvm_runtime_agent.so RuntimeTestApp
```

## Expected Output

Check `jvm_runtime_agent.log` for GC timing and JVM property information.
