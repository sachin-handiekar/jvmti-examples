/**
 * HelloApp.java — Companion test application for Chapter 02 Basic Agent.
 *
 * Usage:
 *   javac HelloApp.java
 *   java -agentpath:./build/libbasic_agent.so=verbose HelloApp
 */
public class HelloApp {
    public static void main(String[] args) {
        System.out.println("HelloApp: Starting...");
        for (int i = 1; i <= 5; i++) {
            System.out.println("HelloApp: Step " + i);
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        System.out.println("HelloApp: Done.");
    }
}
