/**
 * RuntimeTestApp.java — Companion test app for Chapter 07.
 *
 * Creates garbage-collection pressure to trigger GC events.
 */
import java.util.ArrayList;
import java.util.List;

public class RuntimeTestApp {
    public static void main(String[] args) {
        System.out.println("RuntimeTestApp: Generating GC pressure...");

        for (int round = 0; round < 10; round++) {
            List<byte[]> chunks = new ArrayList<>();
            for (int i = 0; i < 1000; i++) {
                chunks.add(new byte[1024]);  // 1 KB each
            }
            chunks.clear();
            System.gc();  // Suggest GC
            System.out.println("  Round " + (round + 1) + " complete.");
        }

        System.out.println("RuntimeTestApp: Done. Check jvm_runtime_agent.log.");
    }
}
