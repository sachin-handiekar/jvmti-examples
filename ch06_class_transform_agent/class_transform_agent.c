#include <jvmti.h>
#include <stdio.h>
#include <string.h>

// Target class to transform
#define TARGET_CLASS "com/example/TargetClass"

// Example: Replace bytecode with a pre-instrumented version (for demo)
// In a real scenario, use ASM or similar to modify the class file dynamically.
// Here, we just log when the class is loaded and could swap in alternate bytes.

static void JNICALL cbClassFileLoadHook(
    jvmtiEnv* jvmti,
    JNIEnv* jni,
    jclass class_being_redefined,
    jobject loader,
    const char* name,
    jobject protection_domain,
    jint class_data_len,
    const unsigned char* class_data,
    jint* new_class_data_len,
    unsigned char** new_class_data
) {
    if (name && strcmp(name, TARGET_CLASS) == 0) {
        printf("[JVMTI] Detected loading of target class: %s\n", name);
        // For demonstration, just log. To actually instrument, you would:
        // 1. Use a bytecode library (ASM) to inject a call to System.out.println at method entry.
        // 2. Replace *new_class_data with the new byte array and set *new_class_data_len.
        // Here, we simply log and pass through the original bytes.
    }
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }

    // Request capability for class retransformation
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_retransform_classes = 1;
    caps.can_generate_all_class_hook_events = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to add capabilities: %d\n", err);
        return JNI_ERR;
    }

    // Register ClassFileLoadHook callback
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.ClassFileLoadHook = &cbClassFileLoadHook;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to set event callbacks: %d\n", err);
        return JNI_ERR;
    }

    // Enable ClassFileLoadHook event
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to enable ClassFileLoadHook event: %d\n", err);
        return JNI_ERR;
    }

    printf("[JVMTI] Class transform agent loaded and ClassFileLoadHook registered.\n");
    return JNI_OK;
}
