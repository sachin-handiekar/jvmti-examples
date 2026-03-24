/**
 * HeapStackTestApp.java — Companion test app for Chapter 05.
 *
 * Creates objects and threads to exercise heap/stack analysis.
 */
import java.util.ArrayList;
import java.util.List;

public class HeapStackTestApp {

    static volatile boolean running = true;

    public static void main(String[] args) throws InterruptedException {
        System.out.println("HeapStackTestApp: Creating objects and threads...");

        // Create objects on the heap
        List<Object> objects = new ArrayList<>();
        for (int i = 0; i < 500; i++) {
            objects.add(new byte[256]);
            objects.add("String-" + i);
        }

        // Create worker threads to give stack depth
        Thread[] workers = new Thread[3];
        for (int i = 0; i < workers.length; i++) {
            final int id = i;
            workers[i] = new Thread(() -> workerBody(id), "Worker-" + i);
            workers[i].start();
        }

        Thread.sleep(500);  // Let workers build up stacks

        running = false;
        for (Thread w : workers) w.join();

        System.out.println("HeapStackTestApp: Done (" + objects.size()
                + " objects created). Check heap_stack_agent.log.");
    }

    static void workerBody(int id) {
        recursiveWork(id, 5);
    }

    static void recursiveWork(int id, int depth) {
        if (depth <= 0 || !running) {
            try { Thread.sleep(200); } catch (InterruptedException e) { }
            return;
        }
        recursiveWork(id, depth - 1);
    }
}
