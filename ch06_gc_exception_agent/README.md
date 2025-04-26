# Chapter 6: Exception and GC Monitoring

This example demonstrates a JVMTI agent that:
- Captures uncaught exceptions and prints the exception class and message
- Registers for GarbageCollectionStart and GarbageCollectionFinish events
- Prints timestamps to measure GC duration
- Logs output to `gc_exception_agent.log`

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
java -agentpath:/path/to/gc_exception_agent.dll -jar YourApp.jar
```

Check `gc_exception_agent.log` for exception and GC event logs.
