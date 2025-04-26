# Chapter 5: Method Entry, Exit, and Instrumentation

This example demonstrates a JVMTI profiler agent that:
- Logs method entry and exit events for methods in the `com.example` package
- Uses JVMTI MethodEntry and MethodExit events
- Optionally counts invocations per method and logs the result on VM shutdown

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
java -agentpath:/path/to/profiler_agent.dll -jar YourApp.jar
```

Check `profiler_agent.log` for method entry/exit logs and invocation counts.
