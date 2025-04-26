# Chapter 11: Memory Management and Resource Cleanup

This example demonstrates a JVMTI agent that:
- Allocates memory using `Allocate` and properly deallocates it with `Deallocate`.
- Logs memory usage to ensure no memory leaks occur.
- Simulates a scenario where cleanup is missed (memory leak) and shows how `Agent_OnUnload` can recover it.

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
java -agentpath:/path/to/memory_cleanup_agent.dll -jar YourApp.jar
```

- Check `memory_cleanup_agent.log` for allocation, deallocation, and cleanup messages.

## What it demonstrates

- The agent allocates two blocks of memory: one is properly deallocated, the other is intentionally leaked.
- On agent unload (`Agent_OnUnload`), the leaked memory is detected and cleaned up, ensuring no memory leaks persist after the agent is unloaded.
- All actions are logged for verification.
