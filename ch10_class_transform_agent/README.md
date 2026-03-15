# Chapter 10 – Class Transform Agent

## Purpose

Demonstrates how to use the **ClassFileLoadHook** event to intercept class loading and potentially transform (instrument) class bytecode before it is loaded into the JVM.

## Key Concepts

- Requesting `can_retransform_classes` and `can_generate_all_class_hook_events` capabilities
- Registering a `ClassFileLoadHook` callback
- Filtering by class name to target specific classes
- Placeholder for bytecode transformation (e.g., injecting logging via ASM)

## Files

| File | Description |
|------|-------------|
| `class_transform_agent.c` | The JVMTI agent (C shared library) |
| `TargetClass.java` | Sample Java class targeted for transformation |
| `CMakeLists.txt` | Build configuration |

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
# Compile the target class
javac -d . TargetClass.java

# Run with the agent loaded
java -agentpath:./build/class_transform_agent -cp . com.example.TargetClass
```

## Expected Output

```
[JVMTI] Class transform agent loaded and ClassFileLoadHook registered.
[JVMTI] Detected loading of target class: com/example/TargetClass
```

## Notes

This example **logs** when the target class is loaded but does not modify the bytecode. In a real-world scenario, you would use a bytecode library (e.g., ASM) to inject instrumentation code and provide the modified bytes via `new_class_data`.
