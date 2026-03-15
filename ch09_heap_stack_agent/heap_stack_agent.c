/**
 * Chapter 09 — Heap Walking & Stack Trace Agent
 *
 * Demonstrates:
 *   - IterateOverInstancesOfClass to count instances of specific classes
 *   - GetAllThreads + GetStackTrace for full stack trace capture
 *   - Writing class histograms and stack traces to log files
 *
 * Uses: common/jvmti_utils.h for JVMTI_CHECK and jvmti_log helpers.
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE "heap_stack_agent.log"
#define MAX_CLASSES 16

/* ------------------------------------------------------------------ */
/*  Class histogram tracking                                          */
/* ------------------------------------------------------------------ */

static struct {
    const char* signature;
    int count;
} class_histogram[MAX_CLASSES] = {
    {"Ljava/lang/String;", 0},
    {"Ljava/lang/Integer;", 0},
    {"Ljava/lang/Object;", 0},
    {"Ljava/util/ArrayList;", 0},
    {"Ljava/util/HashMap;", 0},
};
static int num_tracked_classes = 5;

/**
 * Callback for IterateOverInstancesOfClass — simply increments the
 * user_data counter for each object encountered.
 */
static jvmtiIterationControl JNICALL
heap_instance_callback(jlong class_tag, jlong size,
                       jlong* tag_ptr, void* user_data) {
    int* counter = (int*)user_data;
    (*counter)++;
    return JVMTI_ITERATION_CONTINUE;
}

/**
 * Counts live instances of each tracked class by looking them up in
 * the loaded classes and iterating over heap instances.
 */
static void count_class_instances(jvmtiEnv* jvmti, JNIEnv* jni) {
    jint class_count = 0;
    jclass* classes = NULL;
    jvmtiError err = (*jvmti)->GetLoadedClasses(jvmti, &class_count, &classes);
    if (err != JVMTI_ERROR_NONE) {
        jvmti_log(LOG_FILE, "[Heap] Failed to get loaded classes.");
        return;
    }

    char buf[512];
    for (int i = 0; i < class_count; i++) {
        char* sig = NULL;
        err = (*jvmti)->GetClassSignature(jvmti, classes[i], &sig, NULL);
        if (err != JVMTI_ERROR_NONE || sig == NULL) continue;

        /* Check if this class is one we're tracking */
        for (int j = 0; j < num_tracked_classes; j++) {
            if (strcmp(sig, class_histogram[j].signature) == 0) {
                int count = 0;
                err = (*jvmti)->IterateOverInstancesOfClass(
                    jvmti, classes[i], JVMTI_HEAP_OBJECT_EITHER,
                    heap_instance_callback, &count);
                if (err == JVMTI_ERROR_NONE) {
                    class_histogram[j].count = count;
                }
                break;
            }
        }
        (*jvmti)->Deallocate(jvmti, (unsigned char*)sig);
    }
    (*jvmti)->Deallocate(jvmti, (unsigned char*)classes);

    /* Log the histogram */
    jvmti_log(LOG_FILE, "[Heap] Class Instance Histogram:");
    for (int i = 0; i < num_tracked_classes; i++) {
        snprintf(buf, sizeof(buf), "  %s: %d instances",
                 class_histogram[i].signature, class_histogram[i].count);
        jvmti_log(LOG_FILE, buf);
    }
}

/* ------------------------------------------------------------------ */
/*  Stack trace capture                                               */
/* ------------------------------------------------------------------ */

static void write_stack_traces(jvmtiEnv* jvmti) {
    jint thread_count = 0;
    jthread* threads = NULL;
    jvmtiError err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
    if (err != JVMTI_ERROR_NONE || thread_count == 0) return;

    FILE* fp = fopen("stack_traces.txt", "w");
    if (!fp) return;

    for (int i = 0; i < thread_count; i++) {
        jvmtiFrameInfo frames[32];
        jint count = 0;
        err = (*jvmti)->GetStackTrace(jvmti, threads[i], 0, 32, frames, &count);
        if (err == JVMTI_ERROR_NONE && count > 0) {
            jvmtiThreadInfo tinfo;
            memset(&tinfo, 0, sizeof(tinfo));
            (*jvmti)->GetThreadInfo(jvmti, threads[i], &tinfo);
            fprintf(fp, "Thread: %s\n", tinfo.name ? tinfo.name : "<unknown>");

            for (int j = 0; j < count; j++) {
                char* mname = NULL;
                char* msig = NULL;
                char* csig = NULL;
                jclass decl_class;
                (*jvmti)->GetMethodDeclaringClass(jvmti, frames[j].method, &decl_class);
                (*jvmti)->GetClassSignature(jvmti, decl_class, &csig, NULL);
                (*jvmti)->GetMethodName(jvmti, frames[j].method, &mname, &msig, NULL);
                fprintf(fp, "  at %s.%s%s (bci=%lld)\n",
                        csig ? csig : "?", mname ? mname : "?",
                        msig ? msig : "?", (long long)frames[j].location);
                if (mname) (*jvmti)->Deallocate(jvmti, (unsigned char*)mname);
                if (msig) (*jvmti)->Deallocate(jvmti, (unsigned char*)msig);
                if (csig) (*jvmti)->Deallocate(jvmti, (unsigned char*)csig);
            }
            fprintf(fp, "\n");
            if (tinfo.name) (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
        }
    }
    fclose(fp);
    if (threads) (*jvmti)->Deallocate(jvmti, (unsigned char*)threads);
    jvmti_log(LOG_FILE, "[Heap] Stack traces written to stack_traces.txt");
}

/* ------------------------------------------------------------------ */
/*  JVMTI callbacks and Agent_OnLoad                                  */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMInit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    count_class_instances(jvmti, jni);
    write_stack_traces(jvmti);
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    /* Request heap and stack trace capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_tag_objects = 1;
    caps.can_generate_object_free_events = 1;
    JVMTI_CHECK((*jvmti)->AddCapabilities(jvmti, &caps),
                "Failed to add capabilities");

    /* Register VMInit callback */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &cbVMInit;
    JVMTI_CHECK((*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks)),
                "Failed to set event callbacks");

    /* Enable VMInit event */
    JVMTI_CHECK((*jvmti)->SetEventNotificationMode(
                    jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL),
                "Failed to enable VMInit event");

    printf("[JVMTI] Heap/stack agent loaded and VMInit callback registered.\n");
    return JNI_OK;
}
