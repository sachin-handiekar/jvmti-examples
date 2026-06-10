# Chapter 6 — Class Transformation and Bytecode Instrumentation

Demonstrates how to use the **ClassFileLoadHook** event to intercept class loading and
potentially transform (instrument) class bytecode before it is loaded into the JVM.

## Key Concepts

- Requesting `can_retransform_classes` and `can_generate_all_class_hook_events` capabilities
- Registering a `ClassFileLoadHook` callback
- Filtering by class name to target specific classes (never transform `java/*`, `jdk/*`,
  or `sun/*` classes — see the book's §6.3 safety rule)
- Placeholder for bytecode transformation (e.g., injecting logging via ASM — §6.5)

## Files

| File | Description |
|------|-------------|
| `class_transform_agent.c` | The JVMTI agent (C shared library) |
| `TargetClass.java` | Sample Java class targeted for transformation (runnable) |
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
java -agentpath:./build/libclass_transform_agent.so -cp . com.example.TargetClass
```

(On Windows use `build\class_transform_agent.dll`, on macOS the `.dylib`.)

## Expected Output

```
[JVMTI] Class transform agent loaded and ClassFileLoadHook registered.
[JVMTI] Detected loading of target class: com/example/TargetClass
TargetClass started
Original foo
Original bar
TargetClass finished
```

## Notes

This example **logs** when the target class is loaded but does not modify the bytecode.
To actually instrument it, you would:

1. Use a bytecode library (e.g., [ASM](https://asm.ow2.io/)) to inject instrumentation
2. Allocate the output buffer with `(*jvmti)->Allocate()` — **never** `malloc()`;
   the JVM deallocates it for you (book §6.4)
3. Set `*new_class_data` and `*new_class_data_len` to return the modified bytes

See the book's §6.5 for the full JNI-to-ASM bridge, including the mandatory
`ExceptionCheck`/`ExceptionClear` handling and the `VM_INIT` phase guard.
