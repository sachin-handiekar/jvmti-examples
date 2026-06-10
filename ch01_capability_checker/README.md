# Capability Checker JVMTI Agent

This JVMTI agent prints a greeting message on load and lists a representative set of
the potential capabilities supported by the current JVM (via `GetPotentialCapabilities`).
See the book's Appendix A for the full capabilities reference.

## Build Instructions

```bash
mkdir build && cd build
cmake ..
make
```

## Run Instructions

Use the following command to run the agent with a Java application:

```bash
java -agentpath:/full/path/to/libcapability_checker.so -version
```

## Sample Output

```
[capability-checker] Agent loaded.
--- Potential JVMTI Capabilities ---
can_tag_objects: 1
can_generate_field_modification_events: 1
can_generate_method_entry_events: 1
...
```

## Notes
- Make sure the path to the shared library is absolute.
- Use `nm` or `objdump` to verify `Agent_OnLoad` symbol exists.
