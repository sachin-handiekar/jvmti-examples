# Learning JVMTI -- Code Examples

This repository contains code examples for the book **[Learning JVMTI](https://www.leanpub.com/learning-jvmti/)**.

Each chapter directory contains a self-contained JVMTI agent with a companion Java test application, CMake build file, and chapter-specific README.

## Repository Structure

| Chapter | Directory                         | Focus                                                      |
|---------|------------------------------------|------------------------------------------------------------|
| 1       | ch01_capability_checker            | List potential JVMTI capabilities of the JVM               |
| 2       | ch02_basic_agent                   | Agent lifecycle, options parsing, event registration       |
| 3       | ch03_event_registration            | Capabilities, event callbacks, method/thread tracking      |
| 4       | ch04_thread_class_inspection       | Thread dump: states, stack walking, line numbers           |
| 5       | ch05_heap_stack_agent              | Heap walking with IterateThroughHeap, stack analysis       |
| 6       | ch06_class_transform_agent         | Class transformation and bytecode instrumentation          |
| 7       | ch07_jvm_runtime_agent             | GC events with GetTime, system properties, object sizing   |
| 8       | ch08_exception_agent               | Exception/ExceptionCatch callbacks, filtering, stack traces |
| 9       | ch09_advanced_techniques           | Raw monitors, TLS, reentrancy guards, thread-safe logging  |
| 10      | ch10_profiler_agent                | Minimal sampling profiler with flamegraph output           |
| 11      | ch11_deployment_agent              | Production agent: OnLoad + OnAttach, config, log rotation  |
| 12      | ch12_stability_agent               | Security & stability: capability degradation, safe shutdown, signal chaining |
| 13      | ch13_allocation_tracker            | Case study: allocation sampling (SampledObjectAlloc) with JSON output |

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

- On **Linux** the built library is `libagent_name.so`
- On **macOS** it is `libagent_name.dylib`
- On **Windows** it is `agent_name.dll` (no `lib` prefix)
- Always provide the **absolute path** to the built agent library

See the `README.md` in each chapter directory for specific instructions.

## Agent Capabilities Summary

- **Capability Checker** -- prints a representative set of the JVM's potential capabilities
- **Basic Agent** -- demonstrates the complete agent lifecycle (`Agent_OnLoad`/`Agent_OnUnload`) and `key=value` option parsing
- **Event Registration** -- the three-step event model: capabilities, callbacks, notification modes
- **Thread/Class Inspection** -- Java-style thread dump with states, stack frames, and source lines
- **Heap/Stack Analysis** -- walks the heap with the modern `IterateThroughHeap` API, dumps thread stacks
- **Class Transformation** -- demonstrates bytecode interception via ClassFileLoadHook
- **JVM Runtime** -- times GC events with `GetTime`, reads system properties
- **Exception Handling** -- captures exceptions with throw/catch locations, JDK filtering, and safe JNI use
- **Advanced Techniques** -- raw monitors, Thread-Local Storage, reentrancy guards
- **Profiler** -- sampling profiler: dedicated agent thread, `GetStackTrace`, collapsed-stack/flamegraph output
- **Deployment** -- configurable production agent with `Agent_OnAttach`, rotation-safe logging
- **Stability** -- capability degradation, safe shutdown, chained crash handlers
- **Allocation Tracker** -- full case study sampling all allocation paths via `SampledObjectAlloc` (JDK 11+)

## References

- [Official JVMTI Documentation](https://docs.oracle.com/en/java/javase/21/docs/specs/jvmti.html)

## License

These examples are provided under the MIT License. See [LICENSE](LICENSE) for details.
