# Chapter 10 — Building a Minimal Profiler

A minimal **sampling CPU profiler**: a dedicated agent thread periodically snapshots
every Java thread's stack and aggregates identical stacks into collapsed-stack format,
ready for flamegraph visualization. This is the same technique used by async-profiler,
JProfiler, and YourKit.

> **Why sampling, not instrumentation?** Method entry/exit hooks fire for *every* call —
> millions per second — and distort the very timings they measure. Sampling captures
> stacks at fixed intervals (10ms here) for a statistical picture with low overhead.
> See the book's Chapter 10, §10.1.

## How It Works

1. `VMInit` creates a `java.lang.Thread` via JNI and starts it with `RunAgentThread`
2. The sampling loop calls `GetAllThreads` + `GetStackTrace` every 10ms
3. Stacks are collapsed bottom-up into `frame;frame;frame` keys and counted
4. `VMDeath` writes `profiler_output.txt` in flamegraph "collapsed" format

## Building

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

```sh
javac ProfilerTestApp.java
java -agentpath:./build/libprofiler_agent.so ProfilerTestApp
```

(On Windows use `build\profiler_agent.dll`, on macOS `build/libprofiler_agent.dylib`.)

## Generating a Flamegraph

```sh
git clone https://github.com/brendangregg/FlameGraph.git
cat profiler_output.txt | ./FlameGraph/flamegraph.pl > profile.svg
```

## Notes

- `GetStackTrace` is safe and portable but **safepoint-biased** — production profilers
  like async-profiler use the non-standard `AsyncGetCallTrace` HotSpot API to avoid this
- Never call `GetStackTrace` from a signal handler; the dedicated-thread approach used
  here works on every platform
