/**
 * Chapter 08 — Exception Handling Agent
 *
 * Demonstrates:
 *   - Exception and ExceptionCatch callbacks
 *   - Capturing stack traces at exception throw sites
 *   - Correlating throw and catch locations
 *   - Filtering out JDK-internal exceptions (book §8.6 — thousands per
 *     second are normal; always filter by package prefix)
 *   - Safe JNI use inside the Exception callback: every call that can
 *     throw is followed by ExceptionCheck/ExceptionClear, so no pending
 *     exception leaks into the application thread
 *   - Thrown/caught/uncaught summary at VMDeath
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

/* Demo counters — exceptions can be thrown concurrently on multiple
   threads, so these may under-count slightly; Chapter 9 shows how to
   protect shared counters with raw monitors. */
static long thrown_count = 0;
static long caught_count = 0;
static long uncaught_count = 0;

/* Skip JDK-internal exception traffic (class loading alone throws
   constantly). Returns 1 if the signature is a JDK-internal class. */
static int is_jdk_class(const char* class_sig) {
    return class_sig == NULL ||
           strncmp(class_sig, "Ljava/", 6) == 0 ||
           strncmp(class_sig, "Ljavax/", 7) == 0 ||
           strncmp(class_sig, "Ljdk/", 5) == 0 ||
           strncmp(class_sig, "Lsun/", 5) == 0 ||
           strncmp(class_sig, "Lcom/sun/", 9) == 0;
}

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbException(jvmtiEnv *jvmti, JNIEnv *jni,
                                 jthread thread, jmethodID method,
                                 jlocation location, jobject exception,
                                 jmethodID catch_method,
                                 jlocation catch_location) {
    thrown_count++;
    if (catch_method != NULL) caught_count++;
    else                      uncaught_count++;

    /* Get throw-site class first so we can filter early */
    char* throw_class = NULL;
    char* throw_method = NULL;
    jclass decl_class;
    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method,
                                           &decl_class) == JVMTI_ERROR_NONE) {
        (*jvmti)->GetClassSignature(jvmti, decl_class, &throw_class, NULL);
    }

    /* Filter: only log exceptions thrown from application code (§8.6) */
    if (is_jdk_class(throw_class)) {
        if (throw_class)
            (*jvmti)->Deallocate(jvmti, (unsigned char*)throw_class);
        return;
    }

    (*jvmti)->GetMethodName(jvmti, method, &throw_method, NULL, NULL);

    /* Get exception class name */
    jclass exc_class = (*jni)->GetObjectClass(jni, exception);
    char* class_sig = NULL;
    (*jvmti)->GetClassSignature(jvmti, exc_class, &class_sig, NULL);

    /* Get exception message via JNI. Every JNI call that can fail
       leaves a pending exception which MUST be cleared before this
       callback returns — otherwise the application thread continues
       with a pending exception and unrelated code breaks later. */
    const char* msg_str = "<no message>";
    jstring jmsg = NULL;
    const char* utf = NULL;
    jmethodID mid_getMessage = (*jni)->GetMethodID(jni, exc_class,
                                                    "getMessage",
                                                    "()Ljava/lang/String;");
    if ((*jni)->ExceptionCheck(jni)) {
        (*jni)->ExceptionClear(jni);
        mid_getMessage = NULL;
    }
    if (mid_getMessage) {
        jmsg = (jstring)(*jni)->CallObjectMethod(jni, exception, mid_getMessage);
        if ((*jni)->ExceptionCheck(jni)) {
            /* getMessage itself threw — clear and carry on unlogged */
            (*jni)->ExceptionClear(jni);
            jmsg = NULL;
        }
        if (jmsg) {
            utf = (*jni)->GetStringUTFChars(jni, jmsg, NULL);
            if (utf) msg_str = utf;
        }
    }

    /* Determine if caught or uncaught */
    const char* caught_str = (catch_method != NULL) ? "CAUGHT" : "UNCAUGHT";

    char buf[1024];
    snprintf(buf, sizeof(buf),
             "[Exception] %s %s: %s\n"
             "  Thrown at: %s.%s (bci=%lld)",
             caught_str, class_sig ? class_sig : "?", msg_str,
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
            if ((*jvmti)->GetMethodDeclaringClass(jvmti, frames[i].method,
                                                   &fc) == JVMTI_ERROR_NONE) {
                (*jvmti)->GetClassSignature(jvmti, fc, &cn, NULL);
            }
            (*jvmti)->GetMethodName(jvmti, frames[i].method, &fn, NULL, NULL);
            snprintf(buf, sizeof(buf), "    at %s.%s",
                     cn ? cn : "?", fn ? fn : "?");
            jvmti_log(LOG_FILE, buf);
            if (fn) (*jvmti)->Deallocate(jvmti, (unsigned char*)fn);
            if (cn) (*jvmti)->Deallocate(jvmti, (unsigned char*)cn);
        }
    }

    /* Cleanup */
    if (jmsg && utf) (*jni)->ReleaseStringUTFChars(jni, jmsg, utf);
    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
    if (throw_class) (*jvmti)->Deallocate(jvmti, (unsigned char*)throw_class);
    if (throw_method) (*jvmti)->Deallocate(jvmti, (unsigned char*)throw_method);
}

/* Fires when the JVM enters a catch block (§8.3) */
static void JNICALL cbExceptionCatch(jvmtiEnv *jvmti, JNIEnv *jni,
                                      jthread thread, jmethodID method,
                                      jlocation location, jobject exception) {
    char* class_sig = NULL;
    char* method_name = NULL;
    jclass decl_class;

    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method,
                                           &decl_class) == JVMTI_ERROR_NONE) {
        (*jvmti)->GetClassSignature(jvmti, decl_class, &class_sig, NULL);
    }

    /* Same filter as the throw side */
    if (is_jdk_class(class_sig)) {
        if (class_sig)
            (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
        return;
    }

    (*jvmti)->GetMethodName(jvmti, method, &method_name, NULL, NULL);

    char buf[512];
    snprintf(buf, sizeof(buf), "[ExceptionCatch] Handled in %s.%s (bci=%lld)",
             class_sig ? class_sig : "?",
             method_name ? method_name : "?",
             (long long)location);
    jvmti_log(LOG_FILE, buf);

    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
    if (method_name) (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[Exception] Summary: %ld thrown (%ld caught, %ld uncaught) "
             "including JDK-internal exceptions",
             thrown_count, caught_count, uncaught_count);
    jvmti_log(LOG_FILE, buf);
    printf("%s\n", buf);
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] exception_agent loaded (Chapter 08)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    jvmtiError err;

    /* Request exception capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_exception_events = 1;
    err = (*jvmti)->AddCapabilities(jvmti, &caps);
    CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.Exception      = &cbException;
    callbacks.ExceptionCatch = &cbExceptionCatch;
    callbacks.VMDeath        = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Enable events */
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_EXCEPTION, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable Exception");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_EXCEPTION_CATCH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable ExceptionCatch");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");

    jvmti_log(LOG_FILE, "[Exception] Agent loaded — monitoring exceptions.");
    return JNI_OK;
}
