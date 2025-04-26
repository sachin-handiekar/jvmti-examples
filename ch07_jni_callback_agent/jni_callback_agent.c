#include <jvmti.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>

// Utility to call AgentCallback.agentMessage from JNI
static void call_java_callback(JavaVM* vm, const char* msg) {
    JNIEnv* env = NULL;
    // Get a JNIEnv* for this thread
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        printf("[Agent] Unable to get JNIEnv*\n");
        return;
    }
    jclass cls = (*env)->FindClass(env, "AgentCallback");
    if (!cls) {
        printf("[Agent] Could not find AgentCallback class\n");
        return;
    }
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "agentMessage", "(Ljava/lang/String;)V");
    if (!mid) {
        printf("[Agent] Could not find agentMessage method\n");
        return;
    }
    jstring jmsg = (*env)->NewStringUTF(env, msg);
    (*env)->CallStaticVoidMethod(env, cls, mid, jmsg);
    (*env)->DeleteLocalRef(env, jmsg);
    (*env)->DeleteLocalRef(env, cls);
}

static void JNICALL cbVMInit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    JavaVM* vm = NULL;
    if ((*jni)->GetJavaVM(jni, &vm) != JNI_OK) {
        printf("[Agent] Could not get JavaVM pointer\n");
        return;
    }
    call_java_callback(vm, "Hello from native JVMTI agent!");
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }

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

    printf("[JVMTI] JNI callback agent loaded and VMInit callback registered.\n");
    return JNI_OK;
}
