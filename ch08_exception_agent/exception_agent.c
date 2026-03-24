/**
 * Chapter 08 — Exception Handling Agent
 *
 * Demonstrates:
 *   - Exception and ExceptionCatch callbacks
 *   - Capturing stack traces at exception throw sites
 *   - Correlating throw and catch locations
 *   - Filtering to log only uncaught or specific exceptions
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac ExceptionTestApp.java
 *   java -agentpath:./build/libexception_agent.so ExceptionTestApp
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE "exception_agent.log"

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbException(jvmtiEnv *jvmti, JNIEnv *jni,
                                 jthread thread, jmethodID method,
                                 jlocation location, jobject exception,
                                 jmethodID catch_method,
                                 jlocation catch_location) {
    /* Get exception class name */
    jclass exc_class = (*jni)->GetObjectClass(jni, exception);
    char* class_sig = NULL;
    if ((*jvmti)->GetClassSignature(jvmti, exc_class,
                                     &class_sig, NULL) != JVMTI_ERROR_NONE) {
        return;
    }

    /* Get exception message */
    jmethodID mid_getMessage = (*jni)->GetMethodID(jni, exc_class,
                                                    "getMessage",
                                                    "()Ljava/lang/String;");
    const char* msg_str = "<no message>";
    jstring jmsg = NULL;
    if (mid_getMessage) {
        jmsg = (jstring)(*jni)->CallObjectMethod(jni, exception, mid_getMessage);
        if (jmsg) {
            msg_str = (*jni)->GetStringUTFChars(jni, jmsg, NULL);
        }
    }

    /* Get throw location details */
    char* throw_class = NULL;
    char* throw_method = NULL;
    jclass decl_class;
    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method,
                                           &decl_class) == JVMTI_ERROR_NONE) {
        (*jvmti)->GetClassSignature(jvmti, decl_class, &throw_class, NULL);
    }
    (*jvmti)->GetMethodName(jvmti, method, &throw_method, NULL, NULL);

    /* Determine if caught or uncaught */
    const char* caught_str = (catch_method != NULL) ? "CAUGHT" : "UNCAUGHT";

    char buf[1024];
    snprintf(buf, sizeof(buf),
             "[Exception] %s %s: %s\n"
             "  Thrown at: %s.%s (bci=%lld)",
             caught_str, class_sig, msg_str,
             throw_class ? throw_class : "?",
             throw_method ? throw_method : "?",
             (long long)location);
    jvmti_log(LOG_FILE, buf);

    /* If caught, also log catch location */
    if (catch_method != NULL) {
        char* catch_class_sig = NULL;
        char* catch_method_name = NULL;
        jclass catch_decl;
        if ((*jvmti)->GetMethodDeclaringClass(jvmti, catch_method,
                                               &catch_decl) == JVMTI_ERROR_NONE) {
            (*jvmti)->GetClassSignature(jvmti, catch_decl,
                                         &catch_class_sig, NULL);
        }
        (*jvmti)->GetMethodName(jvmti, catch_method,
                                 &catch_method_name, NULL, NULL);
        snprintf(buf, sizeof(buf), "  Caught at: %s.%s (bci=%lld)",
                 catch_class_sig ? catch_class_sig : "?",
                 catch_method_name ? catch_method_name : "?",
                 (long long)catch_location);
        jvmti_log(LOG_FILE, buf);
        if (catch_class_sig)
            (*jvmti)->Deallocate(jvmti, (unsigned char*)catch_class_sig);
        if (catch_method_name)
            (*jvmti)->Deallocate(jvmti, (unsigned char*)catch_method_name);
    }

    /* Capture stack trace at throw site */
    jvmtiFrameInfo frames[16];
    jint frame_count = 0;
    if ((*jvmti)->GetStackTrace(jvmti, thread, 0, 16,
                                 frames, &frame_count) == JVMTI_ERROR_NONE
        && frame_count > 0) {
        jvmti_log(LOG_FILE, "  Stack trace:");
        for (int i = 0; i < frame_count; i++) {
            char* fn = NULL;
            char* cn = NULL;
            jclass fc;
            (*jvmti)->GetMethodDeclaringClass(jvmti, frames[i].method, &fc);
            (*jvmti)->GetClassSignature(jvmti, fc, &cn, NULL);
            (*jvmti)->GetMethodName(jvmti, frames[i].method, &fn, NULL, NULL);
            snprintf(buf, sizeof(buf), "    at %s.%s",
                     cn ? cn : "?", fn ? fn : "?");
            jvmti_log(LOG_FILE, buf);
            if (fn) (*jvmti)->Deallocate(jvmti, (unsigned char*)fn);
            if (cn) (*jvmti)->Deallocate(jvmti, (unsigned char*)cn);
        }
    }

    /* Cleanup */
    if (jmsg && msg_str) (*jni)->ReleaseStringUTFChars(jni, jmsg, msg_str);
    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
    if (throw_class) (*jvmti)->Deallocate(jvmti, (unsigned char*)throw_class);
    if (throw_method) (*jvmti)->Deallocate(jvmti, (unsigned char*)throw_method);
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] exception_agent loaded (Chapter 08)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    /* Request exception capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_exception_events = 1;
    CHECK_JVMTI_ERROR(
        (*jvmti)->AddCapabilities(jvmti, &caps),
        "Failed to add capabilities");

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.Exception = &cbException;
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks)),
        "Failed to set event callbacks");

    /* Enable events */
    CHECK_JVMTI_ERROR(
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                           JVMTI_EVENT_EXCEPTION, NULL),
        "Failed to enable Exception event");

    jvmti_log(LOG_FILE, "[Exception] Agent loaded — monitoring exceptions.");
    return JNI_OK;
}
