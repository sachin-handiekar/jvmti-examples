/**
 * CapabilityTestApp.java — Companion test app for Chapter 01.
 *
 * A minimal Java application used to load and test the capability checker agent.
 * The agent prints all supported JVMTI capabilities during Agent_OnLoad.
 */
public class CapabilityTestApp {
    public static void main(String[] args) {
        System.out.println("CapabilityTestApp: Agent capabilities printed above.");
        System.out.println("CapabilityTestApp: Done.");
    }
}
