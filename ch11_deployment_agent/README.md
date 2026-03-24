# Chapter 12: Writing Production-Ready Agents

This example demonstrates a configurable, production-style JVMTI agent that:
- Accepts runtime arguments to control which events to enable and which package to monitor.
- Stores logs to a file (`prod_agent.log`) and rotates them when they reach a size limit.
- Demonstrates error handling and state tracking.

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

Run your Java application with the agent, passing configuration options as a comma-separated string:

```sh
java -agentpath:/path/to/prod_agent.dll=package=com/example,method_entry,method_exit,thread_events -jar YourApp.jar
```

### Available options:
- `package=<name>`: Only monitor methods in the given package (default: `com/example`).
- `method_entry`: Enable method entry event logging.
- `method_exit`: Enable method exit event logging.
- `thread_events`: Enable thread start/end event logging.

## Log Rotation
- Logs are written to `prod_agent.log`. When the file exceeds 10KB (demo value), it is rotated (`prod_agent.log.1`, etc., up to 3 rotations).

## Error Handling & State Tracking
- The agent logs errors (e.g., failed capability requests, callback registration) to the log file.
- State (enabled events, monitored package, log rotation) is tracked in agent variables and reflected in the logs.

## What it demonstrates
- How to parse agent options at runtime for flexible configuration.
- How to implement log rotation for production use.
- How to handle errors robustly and track agent state.
