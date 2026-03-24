/**
 * ExceptionTestApp.java — Companion test app for Chapter 08.
 *
 * Throws various exceptions (caught and uncaught) to exercise the agent.
 */
@SuppressWarnings("unused")
public class ExceptionTestApp {

    public static void main(String[] args) {
        System.out.println("ExceptionTestApp: Testing exception handling...");

        // Test 1: Caught exception
        try {
            riskyOperation();
        } catch (IllegalArgumentException e) {
            System.out.println("  Caught: " + e.getMessage());
        }

        // Test 2: Nested exceptions
        try {
            outerMethod();
        } catch (RuntimeException e) {
            System.out.println("  Caught outer: " + e.getMessage());
        }

        // Test 3: NumberFormatException (caught)
        try {
            Integer.parseInt("not-a-number");  // triggers NumberFormatException
        } catch (NumberFormatException e) {
            System.out.println("  Caught: NumberFormatException");
        }

        // Test 4: NullPointerException (caught)
        try {
            ((String) null).length();  // triggers NullPointerException
        } catch (NullPointerException e) {
            System.out.println("  Caught: NullPointerException");
        }

        System.out.println("ExceptionTestApp: Done. Check exception_agent.log.");
    }

    static void riskyOperation() {
        throw new IllegalArgumentException("test exception from riskyOperation");
    }

    static void outerMethod() {
        try {
            innerMethod();
        } catch (ArithmeticException e) {
            throw new RuntimeException("wrapped: " + e.getMessage(), e);
        }
    }

    static void innerMethod() {
        @SuppressWarnings("unused")
        int x = 1 / 0;  // triggers ArithmeticException
    }
}
