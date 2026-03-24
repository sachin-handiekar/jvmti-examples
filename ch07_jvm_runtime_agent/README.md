# Chapter 07 — JVM Runtime Interaction Agent

Demonstrates JVM runtime inspection and GC monitoring:

- **GC events**: `GarbageCollectionStart` / `GarbageCollectionFinish` with timing
- **System properties**: Reading `java.vm.name`, `java.vm.version`, etc.
- **Object sizing**: Using `GetObjectSize` to measure object memory footprint

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
