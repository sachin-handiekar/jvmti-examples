#include <jvmti.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CLASSES 16

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
static int num_classes = 5;

static void log_histogram() {
    FILE* fp = fopen("heap_histogram.log", "w");
    if (!fp) return;
    fprintf(fp, "Class Histogram:\n");
    for (int i = 0; i < num_classes; ++i) {
        fprintf(fp, "%s: %d\n", class_histogram[i].signature, class_histogram[i].count);
    }
    fclose(fp);
}

static jint JNICALL heap_object_callback(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data) {
    jvmtiEnv* jvmti = (jvmtiEnv*)user_data;
    jclass klass = NULL;
    char* sig = NULL;
    if ((*jvmti)->GetTaggedObject(jvmti, *tag_ptr, &klass) == JVMTI_ERROR_NONE && klass) {
        if ((*jvmti)->GetClassSignature(jvmti, klass, &sig, NULL) == JVMTI_ERROR_NONE && sig) {
            for (int i = 0; i < num_classes; ++i) {
                if (strcmp(sig, class_histogram[i].signature) == 0) {
                    class_histogram[i].count++;
                    break;
                }
            }
            (*jvmti)->Deallocate(jvmti, (unsigned char*)sig);
        }
    }
    return JVMTI_VISIT_OBJECTS;
}

static void count_classes(jvmtiEnv* jvmti) {
    // Use IterateThroughHeap to count class instances
    (*jvmti)->IterateThroughHeap(jvmti, 0, NULL, heap_object_callback, jvmti);
    log_histogram();
}

static void write_stack_traces(jvmtiEnv* jvmti) {
    jint thread_count = 0;
    jthread* threads = NULL;
    jvmtiError err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
    if (err != JVMTI_ERROR_NONE || thread_count == 0) return;
    FILE* fp = fopen("stack_traces.txt", "w");
    if (!fp) return;
    for (int i = 0; i < thread_count; ++i) {
        jvmtiFrameInfo frames[32];
        jint count = 0;
        err = (*jvmti)->GetStackTrace(jvmti, threads[i], 0, 32, frames, &count);
        if (err == JVMTI_ERROR_NONE && count > 0) {
            jvmtiThreadInfo tinfo;
            memset(&tinfo, 0, sizeof(tinfo));
            (*jvmti)->GetThreadInfo(jvmti, threads[i], &tinfo);
            fprintf(fp, "Thread: %s\n", tinfo.name ? tinfo.name : "<unknown>");
            for (int j = 0; j < count; ++j) {
                char* mname = NULL;
                char* msig = NULL;
                char* csig = NULL;
                jclass decl_class;
                (*jvmti)->GetMethodDeclaringClass(jvmti, frames[j].method, &decl_class);
                (*jvmti)->GetClassSignature(jvmti, decl_class, &csig, NULL);
                (*jvmti)->GetMethodName(jvmti, frames[j].method, &mname, &msig, NULL);
                fprintf(fp, "  at %s.%s%s (bci=%lld)\n", csig ? csig : "?", mname ? mname : "?", msig ? msig : "?", (long long)frames[j].location);
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
}

static void JNICALL cbVMInit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    count_classes(jvmti);
    write_stack_traces(jvmti);
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }
    // Request heap and stack trace capabilities
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_tag_objects = 1;
    caps.can_generate_object_free_events = 1;
    caps.can_get_owned_monitor_info = 1;
    caps.can_get_current_thread_cpu_time = 1;
    caps.can_get_thread_cpu_time = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to add capabilities: %d\n", err);
        return JNI_ERR;
    }
    // Register VMInit callback
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &cbVMInit;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to set event callbacks: %d\n", err);
        return JNI_ERR;
    }
    // Enable VMInit event
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to enable VMInit event: %d\n", err);
        return JNI_ERR;
    }
    printf("[JVMTI] Heap/stack agent loaded and VMInit callback registered.\n");
    return JNI_OK;
}
