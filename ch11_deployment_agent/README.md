# Chapter 11 — Deploying and Testing JVMTI Agents

A configurable, production-style JVMTI agent demonstrating deployment concerns:

- **Both entry points**: `Agent_OnLoad` (startup) and `Agent_OnAttach` (live attach,
  book §11.4) share one initialization path — the best-practice from §11.6
- **Runtime options** controlling which events to enable and which package to monitor
- **Thread-safe file logging with rotation** (`prod_agent.log`, raw-monitor protected —
  event callbacks fire concurrently on every Java thread)
- **Graceful degradation**: `GetPotentialCapabilities` is consulted before requesting
  capabilities, since some (like method entry/exit) may be unavailable at attach time

## Building

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

### Load at startup

```sh
javac -d . DeployTestApp.java
java -agentpath:./build/libprod_agent.so=package=com/example,method_entry,thread_events \
     -cp . com.example.DeployTestApp
```

(On Windows use `build\prod_agent.dll`, on macOS `build/libprod_agent.dylib`.)

### Attach to a running JVM (Java 9+)

```sh
jps                                                        # find the PID
jcmd <pid> JVMTI.agent_load /abs/path/libprod_agent.so thread_events
```

> **Java 21+ note:** dynamic attach prints a warning by default (JEP 451). Add
> `-XX:+EnableDynamicAgentLoading` to the target JVM to suppress it.

### Available options

| Option | Effect |
|--------|--------|
| `package=<prefix>` | Only monitor methods whose class contains the prefix (default: `com/example`) |
| `method_entry` | Enable method entry logging |
| `method_exit` | Enable method exit logging |
| `thread_events` | Enable thread start/end logging |

## Log Rotation

Logs are written to `prod_agent.log`. When the file exceeds 10 KB (demo value), it is
rotated to `prod_agent.log.1` … `.3`.

## Overhead Warning

`method_entry`/`method_exit` fire for **every** call and are expensive — they exist here
to demonstrate configurability. For CPU profiling, use the sampling approach from
Chapter 10 instead.
