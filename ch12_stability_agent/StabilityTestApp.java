/**
 * StabilityTestApp.java — Companion test app for Chapter 12.
 *
 * Throws and catches exceptions so the stability agent has activity
 * to count, then exits cleanly to exercise the safe-shutdown path.
 */
public class StabilityTestApp {

    static int risky(int i) {
        if (i % 3 == 0) {
            throw new IllegalStateException("every third call fails");
        }
        return i * 2;
    }

    public static void main(String[] args) {
        System.out.println("StabilityTestApp started");

        int handled = 0;
        for (int i = 0; i < 30; i++) {
            try {
                risky(i);
            } catch (IllegalStateException e) {
                handled++;
            }
        }

        System.out.println("StabilityTestApp handled " + handled + " exceptions");
        System.out.println("StabilityTestApp finished");
    }
}
