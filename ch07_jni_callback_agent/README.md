# Chapter 07 – JNI Callback Agent

## Purpose

Demonstrates how a JVMTI native agent can **call back into Java code** using JNI. The agent loads at VM startup, and during `VMInit` it locates a Java class (`AgentCallback`) and invokes its static method from native code.

## Key Concepts

- Using `FindClass` and `GetStaticMethodID` from a JVMTI callback
- Calling Java static methods from C via `CallStaticVoidMethod`
- Managing JNI local references (`NewStringUTF`, `DeleteLocalRef`)

## Files

| File | Description |
|------|-------------|
| `jni_callback_agent.c` | The JVMTI agent (C shared library) |
| `AgentCallback.java` | Java class with a static method called by the agent |
| `CMakeLists.txt` | Build configuration |

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
# Compile the Java helper class
javac AgentCallback.java

# Run with the agent loaded
java -agentpath:./build/jni_callback_agent -cp . AgentCallback
```

## Expected Output

```
[JVMTI] JNI callback agent loaded and VMInit callback registered.
[AgentCallback] Hello from native JVMTI agent!
```
