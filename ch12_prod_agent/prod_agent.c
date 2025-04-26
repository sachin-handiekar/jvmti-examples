#include <jvmti.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOG_FILE_BASE "prod_agent.log"
#define LOG_FILE_MAX_SIZE (1024 * 10) // 10 KB for demo
#define LOG_FILE_ROTATE_COUNT 3

static FILE* log_fp = NULL;
static char monitored_package[256] = "com/example";
static int enable_method_entry = 0;
static int enable_method_exit = 0;
static int enable_thread_events = 0;
static int log_file_index = 0;
static jvmtiEnv* global_jvmti = NULL;

static void rotate_logs() {
    if (log_fp) fclose(log_fp);
    // Remove oldest log if exists
    char oldlog[64];
    snprintf(oldlog, sizeof(oldlog), LOG_FILE_BASE ".%d", LOG_FILE_ROTATE_COUNT);
    remove(oldlog);
    // Shift logs
    for (int i = LOG_FILE_ROTATE_COUNT - 1; i >= 0; --i) {
        char src[64], dst[64];
        snprintf(src, sizeof(src), i == 0 ? LOG_FILE_BASE : LOG_FILE_BASE ".%d", i);
        snprintf(dst, sizeof(dst), LOG_FILE_BASE ".%d", i + 1);
        rename(src, dst);
    }
    log_fp = fopen(LOG_FILE_BASE, "w");
    log_file_index = 0;
}

static void log_msg(const char* msg) {
    if (!log_fp) {
        log_fp = fopen(LOG_FILE_BASE, "a");
        log_file_index = 0;
    }
    if (!log_fp) return;
    fprintf(log_fp, "%s\n", msg);
    fflush(log_fp);
    log_file_index += strlen(msg) + 1;
    if (log_file_index > LOG_FILE_MAX_SIZE) {
        rotate_logs();
    }
}

static void parse_options(const char* options) {
    if (!options) return;
    char* opts = strdup(options);
    char* token = strtok(opts, ",");
    while (token) {
        if (strncmp(token, "package=", 8) == 0) {
            strncpy(monitored_package, token + 8, sizeof(monitored_package) - 1);
        } else if (strcmp(token, "method_entry") == 0) {
            enable_method_entry = 1;
        } else if (strcmp(token, "method_exit") == 0) {
            enable_method_exit = 1;
        } else if (strcmp(token, "thread_events") == 0) {
            enable_thread_events = 1;
        }
        token = strtok(NULL, ",");
    }
    free(opts);
}

static void JNICALL cbMethodEntry(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jmethodID method) {
    char* class_sig = NULL;
    jclass declaring_class;
    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method, &declaring_class) != JVMTI_ERROR_NONE) return;
    if ((*jvmti)->GetClassSignature(jvmti, declaring_class, &class_sig, NULL) != JVMTI_ERROR_NONE) return;
    if (strncmp(class_sig, "L", 1) == 0 && strstr(class_sig, monitored_package)) {
        char* method_name = NULL;
        char* method_sig = NULL;
        if ((*jvmti)->GetMethodName(jvmti, method, &method_name, &method_sig, NULL) == JVMTI_ERROR_NONE) {
            char buf[512];
            snprintf(buf, sizeof(buf), "[Entry] %s.%s%s", class_sig, method_name, method_sig);
            log_msg(buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)method_sig);
        }
    }
    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
}

static void JNICALL cbMethodExit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jmethodID method, jboolean was_popped_by_exception, jvalue return_value) {
    char* class_sig = NULL;
    jclass declaring_class;
    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method, &declaring_class) != JVMTI_ERROR_NONE) return;
    if ((*jvmti)->GetClassSignature(jvmti, declaring_class, &class_sig, NULL) != JVMTI_ERROR_NONE) return;
    if (strncmp(class_sig, "L", 1) == 0 && strstr(class_sig, monitored_package)) {
        char* method_name = NULL;
        char* method_sig = NULL;
        if ((*jvmti)->GetMethodName(jvmti, method, &method_name, &method_sig, NULL) == JVMTI_ERROR_NONE) {
            char buf[512];
            snprintf(buf, sizeof(buf), "[Exit]  %s.%s%s", class_sig, method_name, method_sig);
            log_msg(buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)method_sig);
        }
    }
    if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
}

static void JNICALL cbThreadStart(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    jvmtiThreadInfo tinfo;
    memset(&tinfo, 0, sizeof(tinfo));
    if ((*jvmti)->GetThreadInfo(jvmti, thread, &tinfo) == JVMTI_ERROR_NONE && tinfo.name) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[ThreadStart] %s", tinfo.name);
        log_msg(buf);
        (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
    }
}

static void JNICALL cbThreadEnd(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    jvmtiThreadInfo tinfo;
    memset(&tinfo, 0, sizeof(tinfo));
    if ((*jvmti)->GetThreadInfo(jvmti, thread, &tinfo) == JVMTI_ERROR_NONE && tinfo.name) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[ThreadEnd] %s", tinfo.name);
        log_msg(buf);
        (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
    }
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        fprintf(stderr, "[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }
    global_jvmti = jvmti;
    parse_options(options);
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    if (enable_method_entry) caps.can_generate_method_entry_events = 1;
    if (enable_method_exit) caps.can_generate_method_exit_events = 1;
    if (enable_thread_events) caps.can_generate_thread_events = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        log_msg("[JVMTI] Failed to add capabilities.");
        return JNI_ERR;
    }
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    if (enable_method_entry) callbacks.MethodEntry = &cbMethodEntry;
    if (enable_method_exit) callbacks.MethodExit = &cbMethodExit;
    if (enable_thread_events) {
        callbacks.ThreadStart = &cbThreadStart;
        callbacks.ThreadEnd = &cbThreadEnd;
    }
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        log_msg("[JVMTI] Failed to set event callbacks.");
        return JNI_ERR;
    }
    if (enable_method_entry) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
        if (err != JVMTI_ERROR_NONE) log_msg("[JVMTI] Failed to enable METHOD_ENTRY event.");
    }
    if (enable_method_exit) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, NULL);
        if (err != JVMTI_ERROR_NONE) log_msg("[JVMTI] Failed to enable METHOD_EXIT event.");
    }
    if (enable_thread_events) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, NULL);
        err |= (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, NULL);
        if (err != JVMTI_ERROR_NONE) log_msg("[JVMTI] Failed to enable THREAD events.");
    }
    log_msg("[JVMTI] Production agent loaded and configured.");
    return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
    if (log_fp) {
        log_msg("[JVMTI] Agent unloading, closing log file.");
        fclose(log_fp);
        log_fp = NULL;
    }
}
