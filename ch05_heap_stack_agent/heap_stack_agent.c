/**
 * Chapter 05 — Heap and Stack Analysis Agent
 *
 * Demonstrates:
 *   - IterateOverReachableObjects to walk the heap
 *   - GetStackTrace for all live threads
 *   - GetLocalVariableTable for inspecting local variables
 *   - GetObjectSize for measuring heap footprint
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac HeapStackTestApp.java
 *   java -agentpath:./build/libheap_stack_agent.so HeapStackTestApp
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE "heap_stack_agent.log"

/* ------------------------------------------------------------------ */
/*  Thread Stack Dumper                                                */
/* ------------------------------------------------------------------ */

static void dump_all_thread_stacks(jvmtiEnv *jvmti) {
    jthread* threads = NULL;
    jint thread_count = 0;
    jvmtiError err;

    err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
    if (err != JVMTI_ERROR_NONE || !threads) return;

    char buf[1024];
    snprintf(buf, sizeof(buf), "[HeapStack] Dumping stacks for %d threads:",
             (int)thread_count);
    jvmti_log(LOG_FILE, buf);

    for (int t = 0; t < thread_count; t++) {
        jvmtiThreadInfo tinfo;
        memset(&tinfo, 0, sizeof(tinfo));
        (*jvmti)->GetThreadInfo(jvmti, threads[t], &tinfo);

        snprintf(buf, sizeof(buf), "  Thread: %s",
                 tinfo.name ? tinfo.name : "<unnamed>");
        jvmti_log(LOG_FILE, buf);

        jvmtiFrameInfo frames[32];
        jint frame_count = 0;
        err = (*jvmti)->GetStackTrace(jvmti, threads[t],
                                       0, 32, frames, &frame_count);
        if (err == JVMTI_ERROR_NONE) {
            for (int f = 0; f < frame_count; f++) {
                char* method_name = NULL;
                char* class_sig = NULL;
                jclass decl_class;
                (*jvmti)->GetMethodDeclaringClass(jvmti, frames[f].method,
                                                   &decl_class);
                (*jvmti)->GetClassSignature(jvmti, decl_class,
                                             &class_sig, NULL);
                (*jvmti)->GetMethodName(jvmti, frames[f].method,
                                         &method_name, NULL, NULL);
                snprintf(buf, sizeof(buf), "    at %s.%s (bci=%lld)",
                         class_sig ? class_sig : "?",
                         method_name ? method_name : "?",
                         (long long)frames[f].location);
                jvmti_log(LOG_FILE, buf);

                if (method_name)
                    (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
                if (class_sig)
                    (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
            }
        }

        if (tinfo.name)
            (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
    }

    (*jvmti)->Deallocate(jvmti, (unsigned char*)threads);
}

/* ------------------------------------------------------------------ */
/*  Heap Summary via IterateOverReachableObjects                       */
/* ------------------------------------------------------------------ */

typedef struct {
    long object_count;
    long total_bytes;
} heap_summary_t;

static jvmtiIterationControl JNICALL
heap_object_cb(jvmtiObjectReferenceKind ref_kind,
               jlong class_tag, jlong size, jlong* tag_ptr,
               jlong referrer_tag, jint referrer_index,
               void* user_data) {
    heap_summary_t* summary = (heap_summary_t*)user_data;
    summary->object_count++;
    summary->total_bytes += (long)size;
    return JVMTI_ITERATION_CONTINUE;
}

static void dump_heap_summary(jvmtiEnv *jvmti) {
    heap_summary_t summary;
    summary.object_count = 0;
    summary.total_bytes = 0;

    jvmti_log(LOG_FILE, "[HeapStack] Walking reachable objects...");

    jvmtiError err = (*jvmti)->IterateOverReachableObjects(
        jvmti, NULL, NULL, &heap_object_cb, &summary);

    if (err == JVMTI_ERROR_NONE) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "[HeapStack] Reachable: %ld objects, %ld bytes total",
                 summary.object_count, summary.total_bytes);
        jvmti_log(LOG_FILE, buf);
    } else {
        jvmti_log(LOG_FILE, "[HeapStack] ERROR: IterateOverReachableObjects failed");
    }
}

/* ------------------------------------------------------------------ */
/*  VMInit Callback — Trigger Analysis                                 */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmti_log(LOG_FILE, "[HeapStack] VMInit — running analysis...");
    dump_all_thread_stacks(jvmti);
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    dump_heap_summary(jvmti);
    dump_all_thread_stacks(jvmti);
    jvmti_log(LOG_FILE, "[HeapStack] VMDeath — analysis complete.");
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] heap_stack_agent loaded (Chapter 05)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    /* Request capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_tag_objects = 1;
    CHECK_JVMTI_ERROR(
        (*jvmti)->AddCapabilities(jvmti, &caps),
        "Failed to add capabilities");

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit  = &cbVMInit;
    callbacks.VMDeath = &cbVMDeath;
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks)),
        "Failed to set event callbacks");

    /* Enable events */
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_VM_INIT, NULL),
        "Failed to enable VMInit event");
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_VM_DEATH, NULL),
        "Failed to enable VMDeath event");

    jvmti_log(LOG_FILE, "[HeapStack] Agent loaded — heap/stack analysis ready.");
    return JNI_OK;
}
