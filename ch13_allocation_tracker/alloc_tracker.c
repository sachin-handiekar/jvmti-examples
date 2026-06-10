/**
 * Chapter 13 — Case Study: Allocation Tracker Agent
 *
 * Demonstrates a complete agent that:
 *   - Samples object allocations via the SampledObjectAlloc event
 *     (JEP 331, JDK 11+) — sees ALL allocation paths, including
 *     plain bytecode `new`, on a statistical sampling basis
 *   - Tunes the sampling rate with SetHeapSamplingInterval
 *   - Tracks per-class sample counts and bytes in a table
 *   - Protects shared data with a raw monitor
 *   - Dumps allocation statistics as JSON on VMDeath
 *
 * Why not VMObjectAlloc? As Chapter 5 explains, VMObjectAlloc only
 * fires for allocations the VM performs internally (JNI, reflection) —
 * it misses every `new` in Java bytecode, which is nearly all
 * allocation in a typical app.
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac AllocTestApp.java
 *   java -agentpath:./build/liballoc_tracker.so AllocTestApp
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE      "alloc_tracker.log"
#define JSON_FILE     "allocations.json"
#define MAX_CLASSES   1024

/* Sample roughly every 64 KB of allocation per thread
   (default is 512 KB; lower = more samples, more overhead) */
#define SAMPLING_INTERVAL_BYTES (64 * 1024)

/* ------------------------------------------------------------------ */
/*  Allocation Tracking Data Structure                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    char class_sig[256];
    long count;        /* number of samples, NOT total allocations */
    long total_bytes;  /* bytes of the sampled objects */
} alloc_entry_t;

static alloc_entry_t alloc_table[MAX_CLASSES];
static int alloc_table_size = 0;
static jrawMonitorID alloc_lock = NULL;

static int find_or_add_class(const char* sig) {
    for (int i = 0; i < alloc_table_size; i++) {
        if (strcmp(alloc_table[i].class_sig, sig) == 0) return i;
    }
    if (alloc_table_size < MAX_CLASSES) {
        strncpy(alloc_table[alloc_table_size].class_sig, sig, 255);
        alloc_table[alloc_table_size].class_sig[255] = '\0';
        alloc_table[alloc_table_size].count = 0;
        alloc_table[alloc_table_size].total_bytes = 0;
        return alloc_table_size++;
    }
    return -1;  /* table full */
}

/* ------------------------------------------------------------------ */
/*  JSON Output                                                        */
/* ------------------------------------------------------------------ */

static void dump_json(void) {
    FILE* fp = fopen(JSON_FILE, "w");
    if (!fp) {
        jvmti_log(LOG_FILE, "[Tracker] ERROR: Cannot open JSON output file.");
        return;
    }

    long total_samples = 0, total_bytes = 0;
    for (int i = 0; i < alloc_table_size; i++) {
        total_samples += alloc_table[i].count;
        total_bytes   += alloc_table[i].total_bytes;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"summary\": {\n");
    fprintf(fp, "    \"total_classes\": %d,\n", alloc_table_size);
    fprintf(fp, "    \"sampled_allocations\": %ld,\n", total_samples);
    fprintf(fp, "    \"sampled_bytes\": %ld,\n", total_bytes);
    fprintf(fp, "    \"sampling_interval_bytes\": %d\n", SAMPLING_INTERVAL_BYTES);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"classes\": [\n");
    for (int i = 0; i < alloc_table_size; i++) {
        fprintf(fp, "    {\"class\": \"%s\", \"count\": %ld, \"bytes\": %ld}%s\n",
                alloc_table[i].class_sig,
                alloc_table[i].count,
                alloc_table[i].total_bytes,
                (i < alloc_table_size - 1) ? "," : "");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    fclose(fp);

    char buf[256];
    snprintf(buf, sizeof(buf),
             "[Tracker] JSON report written to %s (%d classes, %ld samples)",
             JSON_FILE, alloc_table_size, total_samples);
    jvmti_log(LOG_FILE, buf);
}

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

/* Fires roughly once per sampling-interval bytes of allocation per
   thread, for ALL allocation paths including bytecode `new`.
   The numbers are samples, not exact counts — multiply sampled bytes
   by the interval ratio for estimates (see book §13.5). */
static void JNICALL cbSampledObjectAlloc(jvmtiEnv *jvmti, JNIEnv *jni,
                                          jthread thread, jobject object,
                                          jclass object_klass, jlong size) {
    char* class_sig = NULL;
    if ((*jvmti)->GetClassSignature(jvmti, object_klass,
                                     &class_sig, NULL) != JVMTI_ERROR_NONE) {
        return;
    }

    (*jvmti)->RawMonitorEnter(jvmti, alloc_lock);
    int idx = find_or_add_class(class_sig);
    if (idx >= 0) {
        alloc_table[idx].count++;
        alloc_table[idx].total_bytes += (long)size;
    }
    (*jvmti)->RawMonitorExit(jvmti, alloc_lock);

    (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
}

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmti_log(LOG_FILE, "[Tracker] VMInit — allocation sampling active.");
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    jvmti_log(LOG_FILE, "[Tracker] VMDeath — dumping allocation report...");

    (*jvmti)->RawMonitorEnter(jvmti, alloc_lock);
    dump_json();

    /* Print summary to console */
    printf("[Tracker] Sampled allocations for %d classes. See %s for details.\n",
           alloc_table_size, JSON_FILE);
    (*jvmti)->RawMonitorExit(jvmti, alloc_lock);

    (*jvmti)->DestroyRawMonitor(jvmti, alloc_lock);
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Tracker] alloc_tracker loaded (Chapter 13 Case Study)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    jvmtiError err;

    /* Create raw monitor for thread-safe access to alloc_table */
    err = (*jvmti)->CreateRawMonitor(jvmti, "alloc_lock", &alloc_lock);
    CHECK_JVMTI_ERROR(jvmti, err, "CreateRawMonitor");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Request capabilities (JDK 11+ for sampled allocation events) */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_sampled_object_alloc_events = 1;
    err = (*jvmti)->AddCapabilities(jvmti, &caps);
    CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");
    if (err != JVMTI_ERROR_NONE) {
        fprintf(stderr, "[Tracker] Sampled allocation events not supported "
                        "on this JVM (requires JDK 11+)\n");
        return JNI_ERR;
    }

    /* Tune the sampling rate (default is 512 KB per thread) */
    err = (*jvmti)->SetHeapSamplingInterval(jvmti, SAMPLING_INTERVAL_BYTES);
    CHECK_JVMTI_ERROR(jvmti, err, "SetHeapSamplingInterval");

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.SampledObjectAlloc = &cbSampledObjectAlloc;
    callbacks.VMInit             = &cbVMInit;
    callbacks.VMDeath            = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Enable events */
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_SAMPLED_OBJECT_ALLOC, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable SampledObjectAlloc");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_INIT, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMInit");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");

    jvmti_log(LOG_FILE, "[Tracker] Agent_OnLoad complete — sampling allocations.");
    return JNI_OK;
}
