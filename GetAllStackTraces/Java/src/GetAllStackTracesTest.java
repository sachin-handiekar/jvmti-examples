public class GetAllStackTracesTest {

    public static void main(String[] args) {
        methodStack1();
    }

    private static void methodStack1() {
        methodStack2();
    }

    private static void methodStack2() {
        methodStack3();
    }

    private static void methodStack3() {
        methodStack4();
    }

    private static void methodStack4() {
        methodStack5();
    }

    private static void methodStack5() {
        exceptionMethod();
    }

    private static void exceptionMethod() {
        try {
            throw new Exception("Throwing an exception...");
        } catch (Exception ex) {
            System.err.println("Exception thrown in Java code...");
        }
    }
}


