#include <jvmti.h>
#include <stdio.h>
#include <string.h>

static unsigned char* leaked_memory = NULL;
static unsigned char* cleaned_memory = NULL;
static jvmtiEnv* global_jvmti = NULL;

static void log_msg(const char* msg) {
    FILE* fp = fopen("memory_cleanup_agent.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

static void allocate_memory(jvmtiEnv* jvmti) {
    // Simulate a memory leak (forgotten deallocation)
    jvmtiError err = (*jvmti)->Allocate(jvmti, 1024, &leaked_memory);
    if (err == JVMTI_ERROR_NONE) {
        log_msg("[JVMTI] Allocated 1024 bytes (leaked_memory), not deallocated yet.");
    }
    // Properly managed allocation
    err = (*jvmti)->Allocate(jvmti, 512, &cleaned_memory);
    if (err == JVMTI_ERROR_NONE) {
        log_msg("[JVMTI] Allocated 512 bytes (cleaned_memory), will deallocate now.");
        (*jvmti)->Deallocate(jvmti, cleaned_memory);
        log_msg("[JVMTI] Deallocated 512 bytes (cleaned_memory).");
        cleaned_memory = NULL;
    }
}

static void JNICALL cbVMInit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    allocate_memory(jvmti);
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }
    global_jvmti = jvmti;
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
    printf("[JVMTI] Memory cleanup agent loaded and VMInit callback registered.\n");
    return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
    if (global_jvmti && leaked_memory) {
        log_msg("[JVMTI] Agent_OnUnload: Cleaning up leaked_memory.");
        (*global_jvmti)->Deallocate(global_jvmti, leaked_memory);
        leaked_memory = NULL;
        log_msg("[JVMTI] Deallocated leaked_memory in Agent_OnUnload.");
    }
}
