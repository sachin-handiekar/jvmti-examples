# Chapter 05 — Heap and Stack Analysis Agent

Provides heap and stack inspection capabilities:

- **Heap walking** with `IterateThroughHeap` (the modern JVMTI 1.1+ API, book §5.2)
  to count live objects and total bytes — each object is visited exactly once
- **Thread stack dumps** with `GetAllThreads` + `GetStackTrace`
- Analysis at both `VMInit` and `VMDeath`
- Requires the `can_tag_objects` capability (heap API prerequisite)

> The older JVMTI 1.0 heap functions (`IterateOverHeap`, `IterateOverReachableObjects`)
> are deprecated — and their reference callbacks fire once per *reference*, not per
> object, which silently miscounts. Use `IterateThroughHeap`; for reachability and
> "who holds this alive?" analysis, use `FollowReferences` (book §5.4).

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac HeapStackTestApp.java
java -agentpath:./build/libheap_stack_agent.so HeapStackTestApp
```

(On Windows use `build\heap_stack_agent.dll`, on macOS `build/libheap_stack_agent.dylib`.)

## Expected Output

Check `heap_stack_agent.log` for the heap summary and thread stack traces:

```
[HeapStack] Heap: 42387 objects, 2847592 bytes total
[HeapStack] Average object size: 67 bytes
```
