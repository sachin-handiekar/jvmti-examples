# Chapter 9: Stack Traces and Heap Walking

This example demonstrates a JVMTI agent that:
- Walks the heap to count the number of instances of common classes (including `java.lang.String`)
- Captures stack traces of all live threads at a particular moment
- Outputs a class histogram (`heap_histogram.log`) and stack traces (`stack_traces.txt`)

## Building

1. Make sure you have a valid JAVA_HOME environment variable pointing to your JDK.
2. Run the following commands:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

Run your Java application with the agent:

```sh
java -agentpath:/path/to/heap_stack_agent.dll -jar YourApp.jar
```

- After running, check `heap_histogram.log` for the class instance counts (histogram).
- Check `stack_traces.txt` for the stack traces of all live threads at the moment of VM startup.

## What it does

- On VMInit, the agent walks the heap and counts the number of instances of several common classes (including `java.lang.String`).
- It also captures and writes the stack traces of all live threads to a file.
- This demonstrates how to use JVMTI's heap walking and stack trace APIs for runtime inspection.
