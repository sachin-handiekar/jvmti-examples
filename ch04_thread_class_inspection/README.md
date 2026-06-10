# Chapter 4 — Inspecting Threads, Stacks, and Methods

A thread-dump agent (book §4.5) that, at `VMInit`:

- Enumerates all live threads with `GetAllThreads` / `GetThreadInfo`
- Reports each thread's state via `GetThreadState` — testing the **specific** flag
  (`SLEEPING`) before the general one (`WAITING`), since sleeping is a sub-state of waiting
- Walks each stack with `GetStackTrace` and resolves frames with `GetMethodName`,
  `GetMethodDeclaringClass`, and `GetClassSignature`
- Maps bytecode indices to source lines with `GetLineNumberTable`
  (capability: `can_get_line_numbers`)
- Logs the loaded-class count as a bonus

Output goes to `thread_class_inspection.log`.

## Building

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

```sh
javac -g InspectionTestApp.java     # -g enables line-number tables
java -agentpath:./build/libthread_class_inspection_agent.so InspectionTestApp
```

(On Windows use `build\thread_class_inspection_agent.dll`, on macOS the `.dylib`.)

## Notes

- The `main` thread's stack appears empty in the dump: the `VM_INIT` callback runs *on*
  the main thread, which is executing native agent code at that moment
- Line numbers require classes compiled with `javac -g`; JDK classes show them because
  the JDK ships debug info
