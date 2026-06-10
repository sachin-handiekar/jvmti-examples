/**
 * Chapter 03 — Capabilities and Event Registration
 *
 * The book's §3.6 "Thread and Method Tracker" example:
 *   - Requests can_generate_method_entry_events (capabilities = permissions)
 *   - Registers MethodEntry, ThreadStart, ThreadEnd, VMInit, VMDeath
 *     callbacks (events = hooks)
 *   - Enables each event with SetEventNotificationMode
 *   - Prints a method-entry total at VMDeath
 *
 * Note: method_count++ is not thread-safe — MethodEntry fires
 * concurrently on every Java thread, so this demo may under-count.
 * Chapter 9 shows how to protect shared counters with raw monitors.
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac EventTestApp.java
 *   java -agentpath:./build/libevent_logger_agent.so EventTestApp
 */

#include <jvmti.h>
#include <stdio.h>
#include <string.h>

static long method_count = 0;

static void log_event(const char* msg) {
    FILE* fp = fopen("event_logger_agent.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    log_event("[JVMTI] VMInit event occurred.");
}

static void JNICALL cbMethodEntry(jvmtiEnv *jvmti, JNIEnv *jni,
                                   jthread thread, jmethodID method) {
    method_count++;
}

static void JNICALL cbThreadStart(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmtiThreadInfo info;
    memset(&info, 0, sizeof(info));
    jvmtiError err = (*jvmti)->GetThreadInfo(jvmti, thread, &info);
    if (err == JVMTI_ERROR_NONE && info.name) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[JVMTI] Thread started: %s", info.name);
        log_event(buf);
        (*jvmti)->Deallocate(jvmti, (unsigned char*)info.name);
    }
}

static void JNICALL cbThreadEnd(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmtiThreadInfo info;
    memset(&info, 0, sizeof(info));
    jvmtiError err = (*jvmti)->GetThreadInfo(jvmti, thread, &info);
    if (err == JVMTI_ERROR_NONE && info.name) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[JVMTI] Thread ended: %s", info.name);
        log_event(buf);
        (*jvmti)->Deallocate(jvmti, (unsigned char*)info.name);
    }
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[JVMTI] VMDeath. Total method entries: %ld", method_count);
    log_event(buf);
    printf("%s\n", buf);
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }

    /* Step 1: request capabilities. MethodEntry needs one;
       ThreadStart/ThreadEnd and the lifecycle events do not. */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_method_entry_events = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to add capabilities: %d\n", err);
        return JNI_ERR;
    }

    /* Step 2: register callback functions */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit      = &cbVMInit;
    callbacks.MethodEntry = &cbMethodEntry;
    callbacks.ThreadStart = &cbThreadStart;
    callbacks.ThreadEnd   = &cbThreadEnd;
    callbacks.VMDeath     = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to set event callbacks: %d\n", err);
        return JNI_ERR;
    }

    /* Step 3: enable each event individually — error codes are not
       bit flags, so check every call separately */
    const jvmtiEvent events[] = {
        JVMTI_EVENT_VM_INIT,
        JVMTI_EVENT_METHOD_ENTRY,
        JVMTI_EVENT_THREAD_START,
        JVMTI_EVENT_THREAD_END,
        JVMTI_EVENT_VM_DEATH,
    };
    for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                                 events[i], NULL);
        if (err != JVMTI_ERROR_NONE) {
            printf("[JVMTI] Failed to enable event %d: %d\n",
                   (int)events[i], err);
            return JNI_ERR;
        }
    }

    printf("[JVMTI] Agent loaded and events registered.\n");
    return JNI_OK;
}
