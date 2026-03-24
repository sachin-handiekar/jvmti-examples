/**
 * Chapter 13 — Case Study: Allocation Tracker Agent
 *
 * Demonstrates a complete, production-quality agent that:
 *   - Monitors object allocation via VMObjectAlloc callback
 *   - Tracks per-class allocation counts using a hash map
 *   - Protects shared data with a raw monitor
 *   - Dumps allocation statistics as JSON on VMDeath
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

/* ------------------------------------------------------------------ */
/*  Allocation Tracking Data Structure                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    char class_sig[256];
    long count;
    long total_bytes;
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

    fprintf(fp, "{\n  \"allocations\": [\n");
    for (int i = 0; i < alloc_table_size; i++) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"class\": \"%s\",\n", alloc_table[i].class_sig);
        fprintf(fp, "      \"count\": %ld,\n", alloc_table[i].count);
        fprintf(fp, "      \"total_bytes\": %ld\n", alloc_table[i].total_bytes);
        fprintf(fp, "    }%s\n", (i < alloc_table_size - 1) ? "," : "");
    }
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"total_classes_tracked\": %d\n", alloc_table_size);
    fprintf(fp, "}\n");
    fclose(fp);

    char buf[256];
    snprintf(buf, sizeof(buf),
             "[Tracker] JSON report written to %s (%d classes)",
             JSON_FILE, alloc_table_size);
    jvmti_log(LOG_FILE, buf);
}

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMObjectAlloc(jvmtiEnv *jvmti, JNIEnv *jni,
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
    jvmti_log(LOG_FILE, "[Tracker] VMInit — allocation tracking active.");
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    jvmti_log(LOG_FILE, "[Tracker] VMDeath — dumping allocation report...");

    (*jvmti)->RawMonitorEnter(jvmti, alloc_lock);
    dump_json();
    (*jvmti)->RawMonitorExit(jvmti, alloc_lock);

    /* Print summary to console */
    printf("[Tracker] Tracked %d classes. See %s for details.\n",
           alloc_table_size, JSON_FILE);
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Tracker] alloc_tracker loaded (Chapter 13 Case Study)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    /* Create raw monitor for thread-safe access to alloc_table */
    CHECK_JVMTI_ERROR(
        (*jvmti)->CreateRawMonitor(jvmti, "alloc_lock", &alloc_lock),
        "Failed to create raw monitor");

    /* Request capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_vm_object_alloc_events = 1;
    CHECK_JVMTI_ERROR(
        (*jvmti)->AddCapabilities(jvmti, &caps),
        "Failed to add capabilities");

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMObjectAlloc = &cbVMObjectAlloc;
    callbacks.VMInit        = &cbVMInit;
    callbacks.VMDeath       = &cbVMDeath;
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks)),
        "Failed to set event callbacks");

    /* Enable events */
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_VM_OBJECT_ALLOC, NULL),
        "Failed to enable VMObjectAlloc event");
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_VM_INIT, NULL),
        "Failed to enable VMInit event");
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_VM_DEATH, NULL),
        "Failed to enable VMDeath event");

    jvmti_log(LOG_FILE, "[Tracker] Agent_OnLoad complete — waiting for allocations.");
    return JNI_OK;
}
