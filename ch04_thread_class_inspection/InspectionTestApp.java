/**
 * InspectionTestApp.java — Companion test app for Chapter 04.
 *
 * Creates worker threads with deep call stacks so the agent's thread
 * dump has interesting frames to resolve. Compile with `javac -g` to
 * include line-number tables.
 */
public class InspectionTestApp {

    public static void methodC() {
        // The agent may capture us here
        System.out.println("  methodC executing");
        try { Thread.sleep(100); } catch (InterruptedException e) { }
    }

    public static void methodB() {
        methodC();
    }

    public static void methodA() {
        methodB();
    }

    public static void main(String[] args) throws InterruptedException {
        System.out.println("InspectionTestApp started");

        // Worker threads with deep stacks
        Thread worker = new Thread(() -> methodA(), "InspectionWorker");
        worker.start();
        worker.join();

        System.out.println("InspectionTestApp finished — see thread_class_inspection.log");
    }
}
