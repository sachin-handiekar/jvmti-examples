# Chapter 8: Interacting with the JVM Runtime

This example demonstrates a JVMTI agent that:
- Uses SuspendThread, ResumeThread, and GetThreadState to pause and inspect running threads
- Prints thread state and name before and after suspending
- Skips system threads for safety
- Logs output to `thread_control_agent.log`

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
java -agentpath:/path/to/thread_control_agent.dll -jar YourApp.jar
```

Check `thread_control_agent.log` for thread state logs.
