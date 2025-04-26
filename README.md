# Learning JVMTI – Code Examples

Note : Work in progress 

This repository contains code examples for the book **[Learning JVMTI](https://www.leanpub.com/learning-jvmti/)**. 

Each chapter directory demonstrates a key concept or feature of the Java Virtual Machine Tool Interface (JVMTI) via a hands-on native agent, with complete build and usage instructions. These examples are designed to teach you how to write, build, and run JVMTI agents in C/C++, and how to use them for profiling, debugging, monitoring, and production use.

## Repository Structure

Each chapter (`chXX_*`) contains a self-contained JVMTI agent example:

| Chapter | Directory                         | Focus/Agent Description                                   |
|---------|------------------------------------|----------------------------------------------------------|
| 1       | ch01_capability_checker            | List all potential JVMTI capabilities of the JVM.         |
| 3       | ch03_event_registration            | Dynamic capability requests, event registration, logging. |
| 4       | ch04_thread_class_inspection       | List loaded classes and threads using JVMTI APIs.         |
| 5       | ch05_profiler_agent                | Method entry/exit logging, basic profiling.               |
| 6       | ch06_gc_exception_agent            | Exception capture, GC event monitoring.                   |
| 7       | ch07_jni_callback_agent            | JNI callback demonstration.                               |
| 8       | ch08_thread_control_agent          | Thread suspension/resume, thread state inspection.        |
| 9       | ch09_heap_stack_agent              | Heap walking, class histograms, stack trace capture.      |
| 10      | ch10_class_transform_agent         | Class transformation and instrumentation.                 |
| 11      | ch11_memory_cleanup_agent          | Memory allocation/deallocation, resource cleanup.         |
| 12      | ch12_prod_agent                    | Production-ready agent: config, logging, error handling.  |

## General Build Instructions

All examples use CMake for cross-platform builds. You need:
- A C/C++ compiler (e.g., GCC, Clang, MSVC)
- CMake (version 3.10+ recommended)
- A JDK with JVMTI headers (set `JAVA_HOME`)

For each chapter/example:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

## Running the Agents

Each agent is loaded into a Java application using the `-agentpath` JVM argument. Example:

```sh
java -agentpath:/full/path/to/lib<agent_name>.so -jar YourApp.jar
```
- On Windows, use `.dll` instead of `.so`.
- On macOS, use `.dylib`.
- Always provide the absolute path to the built agent library.

See the `README.md` in each chapter for specific run instructions and sample outputs.

## Example Agent Capabilities

- **Capability Checker:** Prints all JVMTI capabilities supported by the JVM.
- **Event Registration:** Dynamically requests capabilities, registers for events, logs VM/thread lifecycle.
- **Thread/Class Inspection:** Lists all loaded classes and active threads on VMInit.
- **Profiler Agent:** Logs method entry/exit, counts invocations, basic profiling.
- **GC/Exception Agent:** Captures uncaught exceptions, monitors GC events, logs durations.
- **JNI Callback Agent:** Demonstrates calling back into Java from native code.
- **Thread Control Agent:** Suspends/resumes threads, inspects thread state.
- **Heap/Stack Agent:** Walks the heap, counts class instances, captures stack traces.
- **Class Transform Agent:** Shows class transformation and instrumentation.
- **Memory Cleanup Agent:** Demonstrates memory allocation, deallocation, and leak recovery.
- **Production Agent:** Configurable, production-style agent with logging, log rotation, and error handling.

## References
- [Official JVMTI Documentation](https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html)
- 

## License
These examples are for educational purposes and are provided under the MIT License. See [LICENSE](LICENSE) for details.
