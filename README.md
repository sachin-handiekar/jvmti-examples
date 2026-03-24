# Learning JVMTI -- Code Examples

This repository contains code examples for the book **[Learning JVMTI](https://www.leanpub.com/learning-jvmti/)**.

Each chapter directory contains a self-contained JVMTI agent with a companion Java test application, CMake build file, and chapter-specific README.

## Repository Structure

| Chapter | Directory                         | Focus                                                      |
|---------|------------------------------------|------------------------------------------------------------|
| 1       | ch01_capability_checker            | List all potential JVMTI capabilities of the JVM           |
| 2       | ch02_basic_agent                   | Agent lifecycle, options parsing, event registration       |
| 3       | ch03_event_registration            | Dynamic capability requests, event callbacks, logging      |
| 4       | ch04_thread_class_inspection       | List loaded classes and threads using JVMTI APIs           |
| 5       | ch05_heap_stack_agent              | Heap walking, reachable objects, thread stack analysis      |
| 6       | ch06_class_transform_agent         | Class transformation and bytecode instrumentation          |
| 7       | ch07_jvm_runtime_agent             | GC events, system properties, object sizing                |
| 8       | ch08_exception_agent               | Exception/ExceptionCatch callbacks, stack traces           |
| 9       | ch09_advanced_techniques           | Raw monitors, TLS, reentrancy guards, thread-safe logging  |
| 10      | ch10_profiler_agent                | Method entry/exit logging, invocation counting             |
| 11      | ch11_deployment_agent              | Production-ready agent: config, logging, error handling    |
| 12      | ch12_debugging_agent               | Heap/stack debugging, class histograms                     |
| 13      | ch13_allocation_tracker            | Case study: allocation tracking with JSON output           |

### Shared Infrastructure

| Directory | Purpose                                                          |
|-----------|------------------------------------------------------------------|
| common/   | `jvmti_utils.h` -- shared macros (`CHECK_JVMTI_ERROR`), logging |
| cmake/    | `JvmtiCommon.cmake` -- CMake module for JNI/JVMTI setup         |

## Build Instructions

All examples use CMake for cross-platform builds. Prerequisites:

- A C/C++ compiler (GCC, Clang, or MSVC)
- CMake 3.10+
- A JDK with JVMTI headers (`JAVA_HOME` must be set)

For each chapter:

```sh
cd chXX_example_name
mkdir build && cd build
cmake ..
cmake --build .
```

## Running the Agents

Each agent is loaded using the `-agentpath` JVM argument:

```sh
# Compile the companion Java app
javac TestApp.java

# Run with the agent
java -agentpath:./build/libagent_name.so TestApp
```

- On **Windows**, use `.dll` instead of `.so`
- On **macOS**, use `.dylib` instead of `.so`
- Always provide the **absolute path** to the built agent library

See the `README.md` in each chapter directory for specific instructions.

## Agent Capabilities Summary

- **Capability Checker** -- enumerates all JVMTI capabilities supported by the JVM
- **Basic Agent** -- demonstrates the complete agent lifecycle (`Agent_OnLoad`/`Agent_OnUnload`)
- **Event Registration** -- dynamically requests capabilities and registers for VM/thread events
- **Thread/Class Inspection** -- lists all loaded classes and active threads at VMInit
- **Heap/Stack Analysis** -- walks reachable objects, dumps thread stacks, measures object sizes
- **Class Transformation** -- demonstrates bytecode instrumentation via ClassFileLoadHook
- **JVM Runtime** -- monitors GC events with timing, reads system properties
- **Exception Handling** -- captures exceptions with throw/catch locations and stack traces
- **Advanced Techniques** -- raw monitors, Thread-Local Storage, reentrancy guards
- **Profiler** -- logs method entry/exit, counts invocations
- **Deployment** -- configurable production agent with logging and error handling
- **Debugging** -- heap walking with class histograms, stack analysis
- **Allocation Tracker** -- full case study with per-class tracking and JSON output

## References

- [Official JVMTI Documentation](https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html)

## License

These examples are provided under the MIT License. See [LICENSE](LICENSE) for details.
