package com.example;

/**
 * DeployTestApp.java — Companion test app for Chapter 11.
 *
 * Lives in the com.example package so it matches the agent's default
 * package filter. Creates threads and calls methods to exercise the
 * configurable production agent.
 *
 * Compile:  javac -d . DeployTestApp.java
 * Run:      java -agentpath:./build/libprod_agent.so=package=com/example,method_entry,thread_events \
 *                -cp . com.example.DeployTestApp
 */
public class DeployTestApp {

    static int compute(int n) {
        int total = 0;
        for (int i = 0; i < n; i++) total += i;
        return total;
    }

    public static void main(String[] args) throws InterruptedException {
        System.out.println("DeployTestApp started");

        Thread worker = new Thread(() -> {
            System.out.println("  worker computed: " + compute(1000));
        }, "DeployWorker");
        worker.start();
        worker.join();

        System.out.println("DeployTestApp finished — see prod_agent.log");
    }
}
