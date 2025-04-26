#include <jvmti.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double gc_start_time = 0.0;

static double current_time_sec() {
    return (double)clock() / CLOCKS_PER_SEC;
}

static void log_msg(const char* msg) {
    FILE* fp = fopen("gc_exception_agent.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

static void JNICALL cbException(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread, jmethodID method, jlocation location, jobject exception, jmethodID catch_method, jlocation catch_location) {
    if (catch_method != NULL) return; // Only log uncaught exceptions
    jclass exc_class = (*jni)->GetObjectClass(jni, exception);
    char* class_sig = NULL;
    if ((*jvmti)->GetClassSignature(jvmti, exc_class, &class_sig, NULL) != JVMTI_ERROR_NONE) return;
    jmethodID mid_getMessage = (*jni)->GetMethodID(jni, exc_class, "getMessage", "()Ljava/lang/String;");
    jstring msg = NULL;
    if (mid_getMessage) {
        msg = (jstring)(*jni)->CallObjectMethod(jni, exception, mid_getMessage);
    }
    const char* cmsg = msg ? (*jni)->GetStringUTFChars(jni, msg, NULL) : NULL;
    char buf[1024];
    snprintf(buf, sizeof(buf), "[Exception] Uncaught: %s: %s", class_sig, cmsg ? cmsg : "<no message>");
    log_msg(buf);
    if (cmsg && msg) (*jni)->ReleaseStringUTFChars(jni, msg, cmsg);
    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
}

static void JNICALL cbGCStart(jvmtiEnv* jvmti) {
    gc_start_time = current_time_sec();
    char buf[256];
    snprintf(buf, sizeof(buf), "[GC] Start at %.3f sec", gc_start_time);
    log_msg(buf);
}

static void JNICALL cbGCFinish(jvmtiEnv* jvmti) {
    double end_time = current_time_sec();
    double duration = end_time - gc_start_time;
    char buf[256];
    snprintf(buf, sizeof(buf), "[GC] Finish at %.3f sec (Duration: %.3f sec)", end_time, duration);
    log_msg(buf);
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
    caps.can_generate_exception_events = 1;
    caps.can_generate_garbage_collection_events = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to add capabilities: %d\n", err);
        return JNI_ERR;
    }

    // Register callbacks
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.Exception = &cbException;
    callbacks.GarbageCollectionStart = &cbGCStart;
    callbacks.GarbageCollectionFinish = &cbGCFinish;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to set event callbacks: %d\n", err);
        return JNI_ERR;
    }

    // Enable events
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, NULL);
    err |= (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);
    err |= (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL);
    if (err != JVMTI_ERROR_NONE) {
        printf("[JVMTI] Failed to enable events: %d\n", err);
        return JNI_ERR;
    }

    printf("[JVMTI] GC/Exception agent loaded and events registered.\n");
    return JNI_OK;
}
