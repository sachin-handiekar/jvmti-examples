/**
 * AdvancedTestApp.java — Companion test application for Chapter 09.
 *
 * Creates multiple threads with work to exercise the advanced agent's
 * TLS, reentrancy guard, and raw monitor features.
 */
public class AdvancedTestApp {

    static class Worker implements Runnable {
        private final String name;
        private final int iterations;

        Worker(String name, int iterations) {
            this.name = name;
            this.iterations = iterations;
        }

        @Override
        public void run() {
            for (int i = 0; i < iterations; i++) {
                doWork(i);
            }
            System.out.println(name + " finished.");
        }

        private void doWork(int step) {
            // Generate some method calls for the agent to track
            String result = String.valueOf(step * step);
            result = result.trim();
        }
    }

    public static void main(String[] args) throws Exception {
        System.out.println("AdvancedTestApp: Starting worker threads...");

        Thread t1 = new Thread(new Worker("Worker-A", 500));
        Thread t2 = new Thread(new Worker("Worker-B", 500));
        Thread t3 = new Thread(new Worker("Worker-C", 500));

        t1.start();
        t2.start();
        t3.start();

        t1.join();
        t2.join();
        t3.join();

        System.out.println("AdvancedTestApp: All workers done.");
    }
}
