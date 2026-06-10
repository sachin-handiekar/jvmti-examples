/**
 * Chapter 07 — JVM Runtime Interaction Agent
 *
 * Demonstrates:
 *   - GarbageCollectionStart / GarbageCollectionFinish events with
 *     GetTime-based duration measurement (book §7.3)
 *   - GetSystemProperty for reading JVM properties
 *   - GetAvailableProcessors and GetPhase
 *   - GetObjectSize for measuring object footprint
 *
 * GC callbacks run in a restricted environment: no JNI, and only
 * callback-safe JVMTI functions. They therefore do bookkeeping ONLY
 * (timestamps and counters); the summary is logged at VMDeath.
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac RuntimeTestApp.java
 *   java -agentpath:./build/libjvm_runtime_agent.so RuntimeTestApp
 */

#include "jvmti_utils.h"

#define LOG_FILE "jvm_runtime_agent.log"

/* GC bookkeeping — written only from GC callbacks (which the JVM
   serializes around collections), read at VMDeath */
static jlong gc_start_ns = 0;
static jlong gc_total_ns = 0;
static jlong gc_max_ns = 0;
static long  gc_count = 0;

/* ------------------------------------------------------------------ */
/*  GC Event Callbacks — bookkeeping only (restricted environment)     */
/* ------------------------------------------------------------------ */

static void JNICALL cbGCStart(jvmtiEnv* jvmti) {
    (*jvmti)->GetTime(jvmti, &gc_start_ns);
}

static void JNICALL cbGCFinish(jvmtiEnv* jvmti) {
    jlong end_ns = 0;
    (*jvmti)->GetTime(jvmti, &end_ns);

    jlong duration = end_ns - gc_start_ns;
    gc_total_ns += duration;
    if (duration > gc_max_ns) gc_max_ns = duration;
    gc_count++;
}

/* ------------------------------------------------------------------ */
/*  VMInit — Print JVM Runtime Info                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    char buf[512];
    jvmti_log(LOG_FILE, "[Runtime] VMInit — printing JVM runtime info...");

    /* Read system properties */
    const char* props[] = {"java.vm.name", "java.vm.version",
                           "java.vm.vendor", "java.home"};
    for (int i = 0; i < 4; i++) {
        char* value = NULL;
        jvmtiError err = (*jvmti)->GetSystemProperty(jvmti, props[i], &value);
        if (err == JVMTI_ERROR_NONE && value) {
            snprintf(buf, sizeof(buf), "[Runtime] %s = %s", props[i], value);
            jvmti_log(LOG_FILE, buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)value);
        }
    }

    /* Available processors */
    jint proc_count = 0;
    if ((*jvmti)->GetAvailableProcessors(jvmti, &proc_count) == JVMTI_ERROR_NONE) {
        snprintf(buf, sizeof(buf), "[Runtime] processors = %d", (int)proc_count);
        jvmti_log(LOG_FILE, buf);
    }

    /* Current phase (LIVE here — full JNI and JVMTI available) */
    jvmtiPhase phase;
    if ((*jvmti)->GetPhase(jvmti, &phase) == JVMTI_ERROR_NONE) {
        snprintf(buf, sizeof(buf), "[Runtime] phase = %s",
                 phase == JVMTI_PHASE_LIVE ? "LIVE" : "other");
        jvmti_log(LOG_FILE, buf);
    }

    /* Demonstrate GetObjectSize on the java.lang.String class object */
    jclass string_class = (*jni)->FindClass(jni, "java/lang/String");
    if (string_class) {
        jlong size = 0;
        jvmtiError err = (*jvmti)->GetObjectSize(jvmti, string_class, &size);
        if (err == JVMTI_ERROR_NONE) {
            snprintf(buf, sizeof(buf),
                     "[Runtime] Size of String class object: %lld bytes",
                     (long long)size);
            jvmti_log(LOG_FILE, buf);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  VMDeath — report the GC statistics gathered by the callbacks       */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    char buf[256];
    if (gc_count > 0) {
        snprintf(buf, sizeof(buf),
                 "[GC] %ld collections, total %.3f ms, max %.3f ms, avg %.3f ms",
                 gc_count,
                 gc_total_ns / 1e6,
                 gc_max_ns / 1e6,
                 gc_total_ns / 1e6 / gc_count);
    } else {
        snprintf(buf, sizeof(buf), "[GC] No collections observed.");
    }
    jvmti_log(LOG_FILE, buf);
    printf("%s\n", buf);
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] jvm_runtime_agent loaded (Chapter 07)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    jvmtiError err;

    /* Request GC event capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_garbage_collection_events = 1;
    err = (*jvmti)->AddCapabilities(jvmti, &caps);
    CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.GarbageCollectionStart  = &cbGCStart;
    callbacks.GarbageCollectionFinish = &cbGCFinish;
    callbacks.VMInit                  = &cbVMInit;
    callbacks.VMDeath                 = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Enable events */
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable GC Start");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable GC Finish");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_INIT, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMInit");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");

    jvmti_log(LOG_FILE, "[Runtime] Agent loaded — monitoring GC and runtime.");
    return JNI_OK;
}
