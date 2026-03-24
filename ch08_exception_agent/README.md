# Chapter 08 — Exception Handling Agent

Monitors and logs all Java exceptions with detailed context:

- **Exception callback** with caught/uncaught classification
- **Throw and catch location** details (class, method, BCI)
- **Stack trace capture** at exception throw site
- **Exception message extraction** via JNI

## Build

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```sh
javac ExceptionTestApp.java
java -agentpath:./build/libexception_agent.so ExceptionTestApp
```

## Expected Output

Check `exception_agent.log` for detailed exception reports with stack traces.
