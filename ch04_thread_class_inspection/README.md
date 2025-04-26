# Chapter 4: Thread and Class Inspection

This example demonstrates a JVMTI agent that:
- On VMInit, prints all loaded classes and all active threads
- Uses GetClassSignature to print class signatures
- Uses GetThreadInfo to print thread names
- Logs output to `thread_class_inspection.log`

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
java -agentpath:/path/to/thread_class_inspection_agent.dll -jar YourApp.jar
```

Check `thread_class_inspection.log` for the output.
