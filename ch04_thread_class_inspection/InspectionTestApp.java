/**
 * InspectionTestApp.java — Companion test app for Chapter 04.
 *
 * Loads various classes and creates threads for inspection.
 */
import java.util.HashMap;
import java.util.ArrayList;

public class InspectionTestApp {
    public static void main(String[] args) throws InterruptedException {
        System.out.println("InspectionTestApp: Loading classes and threads...");

        // Use several standard library classes to populate loaded class list
        HashMap<String, Integer> map = new HashMap<>();
        ArrayList<String> list = new ArrayList<>();
        for (int i = 0; i < 100; i++) {
            map.put("key-" + i, i);
            list.add("item-" + i);
        }

        // Create a few threads
        Thread worker = new Thread(() -> {
            System.out.println("  Worker thread running");
            try { Thread.sleep(200); } catch (InterruptedException e) { }
        }, "InspectionWorker");
        worker.start();
        worker.join();

        System.out.println("InspectionTestApp: Created " + map.size()
                + " map entries. Check agent output for class/thread listing.");
    }
}
