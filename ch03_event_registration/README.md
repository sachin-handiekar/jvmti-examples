# Chapter 3 — Capabilities and Events

The book's §3.6 "Thread and Method Tracker": demonstrates the three-step event model —

1. **Request capabilities** with `AddCapabilities` (`can_generate_method_entry_events`;
   thread and lifecycle events need none)
2. **Register callbacks** with `SetEventCallbacks` (`VMInit`, `MethodEntry`,
   `ThreadStart`, `ThreadEnd`, `VMDeath`)
3. **Enable notifications** with `SetEventNotificationMode` — one call per event,
   each checked individually (error codes are not bit flags)

The agent counts method entries and logs thread starts/ends to
`event_logger_agent.log`, printing the method-entry total at `VMDeath`.

> `method_count++` is intentionally not thread-safe — `MethodEntry` fires concurrently
> on every Java thread, so the demo may under-count. Chapter 9 shows raw monitors.

## Building

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

```sh
javac EventTestApp.java
java -agentpath:./build/libevent_logger_agent.so EventTestApp
```

(On Windows use `build\event_logger_agent.dll`, on macOS the `.dylib`.)

Check `event_logger_agent.log` for the thread events, and the console for the
method-entry total at shutdown.
