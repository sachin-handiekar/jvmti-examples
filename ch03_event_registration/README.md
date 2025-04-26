# Chapter 3: Capabilities and Event Registration

This example demonstrates a JVMTI agent that:
- Dynamically requests necessary capabilities
- Registers for VMInit, ThreadStart, and VMDeath events
- Logs each event as it happens

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
java -agentpath:/path/to/event_logger_agent.dll -jar YourApp.jar
```

Check `event_logger_agent.log` for event logs.
