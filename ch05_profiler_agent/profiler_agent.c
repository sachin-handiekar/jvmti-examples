#include <jvmti.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_METHODS 4096
#define MAX_SIGNATURE_LEN 512

// Simple method invocation counter
static struct {
    char signature[MAX_SIGNATURE_LEN];
    int count;
} method_counts[MAX_METHODS];
static int method_count = 0;

static jrawMonitorID lock = NULL;

static void log_msg(const char* msg) {
    FILE* fp = fopen("profiler_agent.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

static int find_or_add_method(const char* sig) {
    for (int i = 0; i < method_count; ++i) {
        if (strcmp(method_counts[i].signature, sig) == 0)
            return i;
    }
    if (method_count < MAX_METHODS) {
        strncpy(method_counts[method_count].signature, sig, MAX_SIGNATURE_LEN-1);
        method_counts[method_count].signature[MAX_SIGNATURE_LEN-1] = '\0';
        method_counts[method_count].count = 0;
        return method_count++;
    }
    return -1;
}

static void JNICALL cbMethodEntry(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jmethodID method) {
    char* class_sig = NULL;
    char* method_name = NULL;
    char* method_sig = NULL;
    jclass declaring_class;
    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method, &declaring_class) != JVMTI_ERROR_NONE) return;
    if ((*jvmti)->GetClassSignature(jvmti, declaring_class, &class_sig, NULL) != JVMTI_ERROR_NONE) return;
    if (strncmp(class_sig, "Lcom/example/", 13) != 0) {
        if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
        return;
    }
    if ((*jvmti)->GetMethodName(jvmti, method, &method_name, &method_sig, NULL) != JVMTI_ERROR_NONE) {
        if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
        return;
    }
    char full_sig[MAX_SIGNATURE_LEN];
    snprintf(full_sig, sizeof(full_sig), "%s%s%s", class_sig, method_name, method_sig);
    (*jvmti)->RawMonitorEnter(jvmti, lock);
    int idx = find_or_add_method(full_sig);
    if (idx >= 0) method_counts[idx].count++;
    (*jvmti)->RawMonitorExit(jvmti, lock);
    char buf[1024];
    snprintf(buf, sizeof(buf), "[Entry] %s.%s%s", class_sig, method_name, method_sig);
    log_msg(buf);
    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
    if (method_name) (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
    if (method_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)method_sig);
}

static void JNICALL cbMethodExit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jmethodID method, jboolean was_popped_by_exception, jvalue return_value) {
    char* class_sig = NULL;
    char* method_name = NULL;
    char* method_sig = NULL;
    jclass declaring_class;
    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method, &declaring_class) != JVMTI_ERROR_NONE) return;
    if ((*jvmti)->GetClassSignature(jvmti, declaring_class, &class_sig, NULL) != JVMTI_ERROR_NONE) return;
    if (strncmp(class_sig, "Lcom/example/", 13) != 0) {
        if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
        return;
    }
    if ((*jvmti)->GetMethodName(jvmti, method, &method_name, &method_sig, NULL) != JVMTI_ERROR_NONE) {
        if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
        return;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf), "[Exit]  %s.%s%s", class_sig, method_name, method_sig);
    log_msg(buf);
    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
    if (method_name) (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
    if (method_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)method_sig);
}

static void JNICALL cbVMDeath(jvmtiEnv* jvmti, JNIEnv* jni) {
    log_msg("[JVMTI] VMDeath event occurred. Method invocation counts:");
    char buf[1024];
    for (int i = 0; i < method_count; ++i) {
        snprintf(buf, sizeof(buf), "%s: %d", method_counts[i].signature, method_counts[i].count);
        log_msg(buf);
    }
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }

    // Request method entry/exit capabilities
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_method_entry_events = 1;
    caps.can_generate_method_exit_events = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to add capabilities: %d\n", err);
        return JNI_ERR;
    }

    // Create raw monitor for thread safety
    err = (*jvmti)->CreateRawMonitor(jvmti, "profiler_lock", &lock);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to create raw monitor: %d\n", err);
        return JNI_ERR;
    }

    // Register callbacks
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.MethodEntry = &cbMethodEntry;
    callbacks.MethodExit = &cbMethodExit;
    callbacks.VMDeath = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to set event callbacks: %d\n", err);
        return JNI_ERR;
    }

    // Enable events
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
    err |= (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, NULL);
    err |= (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to enable events: %d\n", err);
        return JNI_ERR;
    }

    printf("[JVMTI] Profiler agent loaded and events registered.\n");
    return JNI_OK;
}
