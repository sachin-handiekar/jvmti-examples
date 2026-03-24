/**
 * EventTestApp.java — Companion test app for Chapter 03.
 *
 * Creates threads and classes to exercise the event registration agent.
 */
public class EventTestApp {
    public static void main(String[] args) throws InterruptedException {
        System.out.println("EventTestApp: Creating threads to trigger events...");

        Thread[] threads = new Thread[5];
        for (int i = 0; i < threads.length; i++) {
            final int id = i;
            threads[i] = new Thread(() -> {
                System.out.println("  Worker " + id + " running");
                try { Thread.sleep(100); } catch (InterruptedException e) { }
            }, "EventWorker-" + i);
            threads[i].start();
        }

        for (Thread t : threads) t.join();

        System.out.println("EventTestApp: Done. Check agent output for events.");
    }
}
