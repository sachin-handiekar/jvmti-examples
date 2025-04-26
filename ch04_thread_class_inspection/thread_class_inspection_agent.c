#include <jvmti.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void log_msg(const char* msg) {
    FILE* fp = fopen("thread_class_inspection.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

static void print_all_loaded_classes(jvmtiEnv* jvmti) {
    jint class_count = 0;
    jclass* classes = NULL;
    jvmtiError err = (*jvmti)->GetLoadedClasses(jvmti, &class_count, &classes);
    if (err != JVMTI_ERROR_NONE) {
        log_msg("[JVMTI] Failed to get loaded classes.");
        return;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf), "[JVMTI] Loaded classes (%d):", class_count);
    log_msg(buf);
    for (int i = 0; i < class_count; i++) {
        char* signature = NULL;
        err = (*jvmti)->GetClassSignature(jvmti, classes[i], &signature, NULL);
        if (err == JVMTI_ERROR_NONE && signature) {
            snprintf(buf, sizeof(buf), "  %s", signature);
            log_msg(buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)signature);
        }
    }
    if (classes) {
        (*jvmti)->Deallocate(jvmti, (unsigned char*)classes);
    }
}

static void print_all_threads(jvmtiEnv* jvmti) {
    jint thread_count = 0;
    jthread* threads = NULL;
    jvmtiError err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
    if (err != JVMTI_ERROR_NONE) {
        log_msg("[JVMTI] Failed to get all threads.");
        return;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf), "[JVMTI] Active threads (%d):", thread_count);
    log_msg(buf);
    for (int i = 0; i < thread_count; i++) {
        jvmtiThreadInfo tinfo;
        memset(&tinfo, 0, sizeof(tinfo));
        err = (*jvmti)->GetThreadInfo(jvmti, threads[i], &tinfo);
        if (err == JVMTI_ERROR_NONE && tinfo.name) {
            snprintf(buf, sizeof(buf), "  %s", tinfo.name);
            log_msg(buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
        }
    }
    if (threads) {
        (*jvmti)->Deallocate(jvmti, (unsigned char*)threads);
    }
}

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    log_msg("[JVMTI] VMInit event occurred. Inspecting classes and threads...");
    print_all_loaded_classes(jvmti);
    print_all_threads(jvmti);
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }

    // Request required capabilities (none needed beyond default for this example)

    // Register VMInit callback
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &cbVMInit;
    jvmtiError err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
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

    printf("[JVMTI] Agent loaded and VMInit callback registered.\n");
    return JNI_OK;
}
