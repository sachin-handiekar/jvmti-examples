/**
 * ProfilerTestApp.java — Companion test app for Chapter 10.
 *
 * Provides a CPU-intensive workload so the sampling profiler has
 * interesting stacks to capture.
 */
public class ProfilerTestApp {

    /* Simulate a CPU-intensive workload */
    public static long fibonacci(int n) {
        if (n <= 1) return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    }

    /* Simulate I/O wait */
    public static void simulateIO() throws InterruptedException {
        Thread.sleep(100);
    }

    /* Busy method that should show up in the profile */
    public static void hotMethod() {
        double sum = 0;
        for (int i = 0; i < 1_000_000; i++) {
            sum += Math.sin(i) * Math.cos(i);
        }
    }

    public static void main(String[] args) throws Exception {
        System.out.println("ProfilerTestApp started");

        // CPU-intensive work
        for (int i = 0; i < 5; i++) {
            long result = fibonacci(35);
            System.out.println("  fib(35) = " + result);
        }

        // Hot loop
        for (int i = 0; i < 10; i++) {
            hotMethod();
        }

        // I/O simulation
        simulateIO();

        System.out.println("ProfilerTestApp finished");
    }
}
