# Chapter 02 — Basic JVMTI Agent

This example demonstrates the fundamental building blocks of a JVMTI agent:

- **Agent lifecycle**: `Agent_OnLoad` and `Agent_OnUnload` entry points
- **Environment acquisition**: Getting a `jvmtiEnv*` from `JavaVM*`
- **Options parsing**: Reading comma-separated agent options
- **Error handling**: Using the `CHECK_JVMTI_ERROR` macro
- **Event registration**: VMInit and VMDeath callbacks

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac HelloApp.java
java -agentpath:./build/libbasic_agent.so=verbose HelloApp
```

On Windows use `.dll`, on macOS use `.dylib`.

## Expected Output

```
[Agent] basic_agent loaded (Chapter 02)
[Agent] Verbose mode enabled
HelloApp: Starting...
HelloApp: Step 1
...
HelloApp: Done.
[Agent] basic_agent unloaded
```

Check `basic_agent.log` for detailed event logs.
