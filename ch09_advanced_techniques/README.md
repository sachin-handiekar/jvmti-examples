# Chapter 09 — Advanced Techniques Agent

Demonstrates advanced JVMTI patterns:

- **Raw monitors** for thread-safe logging across callbacks
- **Thread-Local Storage** (TLS) via `SetThreadLocalStorage` / `GetThreadLocalStorage`
- **Reentrancy guard** to prevent recursive callback invocations
- **Per-thread statistics** with cleanup on thread end

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac AdvancedTestApp.java
java -agentpath:./build/libadvanced_agent.so AdvancedTestApp
```

## Expected Output

Check `advanced_agent.log` for thread start/end events and sampled method entry counts per thread.
