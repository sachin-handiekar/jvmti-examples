/**
 * Chapter 05 — Heap and Stack Analysis Agent
 *
 * Demonstrates:
 *   - IterateThroughHeap (the modern JVMTI 1.1+ heap API, book §5.2)
 *     with jvmtiHeapCallbacks to count objects and bytes
 *   - GetStackTrace for all live threads
 *   - can_tag_objects capability (required by the heap API)
 *
 * Note: the older JVMTI 1.0 functions (IterateOverHeap,
 * IterateOverReachableObjects) are deprecated; they also report once
 * per *reference* rather than per object, which silently miscounts.
 * IterateThroughHeap visits each live object exactly once.
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
                if ((*jvmti)->GetMethodDeclaringClass(jvmti, frames[f].method,
                                                       &decl_class) == JVMTI_ERROR_NONE) {
                    (*jvmti)->GetClassSignature(jvmti, decl_class,
                                                 &class_sig, NULL);
                }
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
/*  Heap Summary via IterateThroughHeap (book §5.2 / §5.8)             */
/* ------------------------------------------------------------------ */

typedef struct {
    jlong object_count;
    jlong total_bytes;
} heap_summary_t;

/* Visits every live object exactly once. Keep callbacks fast — the
   heap walk blocks all Java threads (§5.6). Returning
   JVMTI_VISIT_OBJECTS is harmless here (only meaningful for
   FollowReferences); JVMTI_VISIT_ABORT would stop the iteration. */
static jint JNICALL heap_iteration_cb(jlong class_tag, jlong size,
                                       jlong* tag_ptr, jint length,
                                       void* user_data) {
    heap_summary_t* summary = (heap_summary_t*)user_data;
    summary->object_count++;
    summary->total_bytes += size;
    return JVMTI_VISIT_OBJECTS;
}

static void dump_heap_summary(jvmtiEnv *jvmti) {
    heap_summary_t summary = { 0, 0 };

    jvmtiHeapCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.heap_iteration_callback = &heap_iteration_cb;

    jvmti_log(LOG_FILE, "[HeapStack] Walking the heap...");

    jvmtiError err = (*jvmti)->IterateThroughHeap(
        jvmti,
        0,            /* heap_filter: 0 = no filter */
        NULL,         /* klass: NULL = all classes */
        &callbacks,
        &summary);    /* user_data — passed to callback */

    if (err == JVMTI_ERROR_NONE) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "[HeapStack] Heap: %lld objects, %lld bytes total",
                 (long long)summary.object_count,
                 (long long)summary.total_bytes);
        jvmti_log(LOG_FILE, buf);
        if (summary.object_count > 0) {
            snprintf(buf, sizeof(buf),
                     "[HeapStack] Average object size: %lld bytes",
                     (long long)(summary.total_bytes / summary.object_count));
            jvmti_log(LOG_FILE, buf);
        }
    } else {
        CHECK_JVMTI_ERROR(jvmti, err, "IterateThroughHeap");
        jvmti_log(LOG_FILE, "[HeapStack] ERROR: IterateThroughHeap failed");
    }
}

/* ------------------------------------------------------------------ */
/*  VMInit / VMDeath Callbacks — Trigger Analysis                      */
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

    jvmtiError err;

    /* The heap iteration API requires can_tag_objects */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_tag_objects = 1;
    err = (*jvmti)->AddCapabilities(jvmti, &caps);
    CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit  = &cbVMInit;
    callbacks.VMDeath = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Enable events */
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_INIT, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMInit");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");

    jvmti_log(LOG_FILE, "[HeapStack] Agent loaded — heap/stack analysis ready.");
    return JNI_OK;
}
