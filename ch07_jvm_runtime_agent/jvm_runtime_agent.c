/**
 * Chapter 07 — JVM Runtime Interaction Agent
 *
 * Demonstrates:
 *   - GarbageCollectionStart / GarbageCollectionFinish events
 *   - GetSystemProperty for reading JVM properties
 *   - GetObjectSize for measuring object footprint
 *   - Basic JNI callback from native to Java
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac RuntimeTestApp.java
 *   java -agentpath:./build/libjvm_runtime_agent.so RuntimeTestApp
 */

#include "jvmti_utils.h"
#include <time.h>

#define LOG_FILE "jvm_runtime_agent.log"

static double gc_start_time = 0.0;

static double current_time_sec(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

/* ------------------------------------------------------------------ */
/*  GC Event Callbacks                                                 */
/* ------------------------------------------------------------------ */

static void JNICALL cbGCStart(jvmtiEnv* jvmti) {
    gc_start_time = current_time_sec();
    char buf[256];
    snprintf(buf, sizeof(buf), "[GC] Start at %.3f sec", gc_start_time);
    jvmti_log(LOG_FILE, buf);
}

static void JNICALL cbGCFinish(jvmtiEnv* jvmti) {
    double end_time = current_time_sec();
    double duration = end_time - gc_start_time;
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[GC] Finish at %.3f sec (Duration: %.3f sec)",
             end_time, duration);
    jvmti_log(LOG_FILE, buf);
}

/* ------------------------------------------------------------------ */
/*  VMInit — Print JVM Runtime Info                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmti_log(LOG_FILE, "[Runtime] VMInit — printing JVM runtime info...");

    /* Read system properties */
    const char* props[] = {"java.vm.name", "java.vm.version",
                           "java.vm.vendor", "java.home"};
    for (int i = 0; i < 4; i++) {
        char* value = NULL;
        jvmtiError err = (*jvmti)->GetSystemProperty(jvmti, props[i], &value);
        if (err == JVMTI_ERROR_NONE && value) {
            char buf[512];
            snprintf(buf, sizeof(buf), "[Runtime] %s = %s", props[i], value);
            jvmti_log(LOG_FILE, buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)value);
        }
    }

    /* Demonstrate GetObjectSize on a java.lang.String */
    jclass string_class = (*jni)->FindClass(jni, "java/lang/String");
    if (string_class) {
        jlong size = 0;
        jvmtiError err = (*jvmti)->GetObjectSize(jvmti, string_class, &size);
        if (err == JVMTI_ERROR_NONE) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "[Runtime] Size of String class object: %lld bytes",
                     (long long)size);
            jvmti_log(LOG_FILE, buf);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] jvm_runtime_agent loaded (Chapter 07)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    /* Request GC event capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_garbage_collection_events = 1;
    CHECK_JVMTI_ERROR(
        (*jvmti)->AddCapabilities(jvmti, &caps),
        "Failed to add capabilities");

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.GarbageCollectionStart  = &cbGCStart;
    callbacks.GarbageCollectionFinish = &cbGCFinish;
    callbacks.VMInit                  = &cbVMInit;
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks)),
        "Failed to set event callbacks");

    /* Enable events */
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL),
        "Failed to enable GC Start event");
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL),
        "Failed to enable GC Finish event");
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_VM_INIT, NULL),
        "Failed to enable VMInit event");

    jvmti_log(LOG_FILE, "[Runtime] Agent loaded — monitoring GC and runtime.");
    return JNI_OK;
}
