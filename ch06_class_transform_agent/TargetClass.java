package com.example;

public class TargetClass {
    public void foo() {
        System.out.println("Original foo");
    }

    public void bar() {
        System.out.println("Original bar");
    }

    public static void main(String[] args) {
        System.out.println("TargetClass started");
        TargetClass t = new TargetClass();
        t.foo();
        t.bar();
        System.out.println("TargetClass finished");
    }
}
