#include <jvmti.h>
#include <stdio.h>
#include <string.h>

static void log_event(const char* msg) {
    FILE* fp = fopen("event_logger_agent.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    log_event("[JVMTI] VMInit event occurred.");
}

static void JNICALL cbThreadStart(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    log_event("[JVMTI] ThreadStart event occurred.");
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    log_event("[JVMTI] VMDeath event occurred.");
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }

    // Request capabilities
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_thread_events = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to add capabilities: %d\n", err);
        return JNI_ERR;
    }

    // Register event callbacks
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &cbVMInit;
    callbacks.ThreadStart = &cbThreadStart;
    callbacks.VMDeath = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to set event callbacks: %d\n", err);
        return JNI_ERR;
    }

    // Enable events
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
    err |= (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, NULL);
    err |= (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to enable events: %d\n", err);
        return JNI_ERR;
    }

    printf("[JVMTI] Agent loaded and events registered.\n");
    return JNI_OK;
}
