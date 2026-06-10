# Chapter 12 — Security and Stability

A JVMTI agent runs as **native code inside the JVM process** — a single null pointer
dereference crashes the entire application. This example demonstrates the defensive
patterns from Chapter 12 that separate prototype agents from production-ready ones.

## What It Demonstrates

| Pattern | Where |
|---------|-------|
| **Defensive capability check** — `GetPotentialCapabilities` + graceful degradation when a feature is unavailable | `Agent_OnLoad` |
| **Safe shutdown** — `shutting_down` flag, disable events *before* cleanup, destroy raw monitors last | `cbVMDeath` |
| **Crash-handler chaining** — save and delegate to the JVM's SIGSEGV handler (POSIX) / return `EXCEPTION_CONTINUE_SEARCH` (Windows SEH) | `install_crash_handlers`, called from `VMInit` |

> **Why chaining matters:** HotSpot *deliberately* triggers SIGSEGV during normal
> operation — implicit null checks in JIT-compiled code and safepoint polling both rely
> on it. An agent that replaces the JVM's handler (e.g., logs and `_exit()`s) will kill a
> perfectly healthy JVM almost immediately. Handlers are installed from `VMInit` (after
> the JVM has installed its own) so the saved "previous" handler is the JVM's. In
> production, prefer `libjsig`:
>
> ```sh
> LD_PRELOAD=$JAVA_HOME/lib/libjsig.so java -agentpath:./build/libstability_agent.so App
> ```

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac StabilityTestApp.java
java -agentpath:./build/libstability_agent.so StabilityTestApp
```

(On Windows use `build\stability_agent.dll`, on macOS `build/libstability_agent.dylib`.)

## Expected Output

```
[Agent] stability_agent loaded (Chapter 12)
StabilityTestApp started
StabilityTestApp handled 10 exceptions
StabilityTestApp finished
[Stability] VMDeath — NN exceptions observed.
```

(`NN` is larger than 10 — the JDK itself throws and catches exceptions internally
during class loading. Chapter 8 covers filtering.)
