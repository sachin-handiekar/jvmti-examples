/**
 * AllocTestApp.java — Companion test application for Chapter 13 Case Study.
 *
 * Creates various objects to exercise the allocation tracker agent.
 */
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class AllocTestApp {

    public static void main(String[] args) {
        System.out.println("AllocTestApp: Starting allocation-heavy workload...");

        // Create many Strings
        List<String> strings = new ArrayList<>();
        for (int i = 0; i < 1000; i++) {
            strings.add("String-" + i);
        }

        // Create many boxed Integers
        List<Integer> ints = new ArrayList<>();
        for (int i = 0; i < 500; i++) {
            ints.add(Integer.valueOf(i));
        }

        // Create some maps
        Map<String, Integer> map = new HashMap<>();
        for (int i = 0; i < 200; i++) {
            map.put("key-" + i, i);
        }

        // Create a few custom objects
        for (int i = 0; i < 100; i++) {
            Object[] arr = new Object[10];
            arr[0] = new StringBuilder("item-" + i);
        }

        System.out.println("AllocTestApp: Created " + strings.size()
                + " strings, " + ints.size() + " integers, "
                + map.size() + " map entries.");
        System.out.println("AllocTestApp: Done. Check allocations.json.");
    }
}
