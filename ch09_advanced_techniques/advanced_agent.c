/**
 * Chapter 09 — Advanced Techniques Agent
 *
 * Demonstrates:
 *   - Raw monitors for thread synchronization
 *   - Thread-Local Storage (TLS) with SetThreadLocalStorage / GetThreadLocalStorage
 *   - Reentrancy guard to prevent recursive callback invocations
 *   - Multiple event callbacks using advanced techniques
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac AdvancedTestApp.java
 *   java -agentpath:./build/libadvanced_agent.so AdvancedTestApp
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE "advanced_agent.log"

/* ------------------------------------------------------------------ */
/*  Raw Monitor                                                        */
/* ------------------------------------------------------------------ */

static jrawMonitorID agent_lock = NULL;

static void agent_lock_enter(jvmtiEnv* jvmti) {
    (*jvmti)->RawMonitorEnter(jvmti, agent_lock);
}

static void agent_lock_exit(jvmtiEnv* jvmti) {
    (*jvmti)->RawMonitorExit(jvmti, agent_lock);
}

/* ------------------------------------------------------------------ */
/*  Thread-Local Storage (TLS)                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    int method_entry_count;
    int reentrancy_guard;       /* 1 = inside callback, prevents recursion */
} thread_data_t;

static thread_data_t* get_thread_data(jvmtiEnv* jvmti, jthread thread) {
    thread_data_t* data = NULL;
    jvmtiError err = (*jvmti)->GetThreadLocalStorage(jvmti, thread,
                                                      (void**)&data);
    if (err != JVMTI_ERROR_NONE || data == NULL) {
        /* First call for this thread — allocate and set TLS */
        data = (thread_data_t*)calloc(1, sizeof(thread_data_t));
        if (data) {
            (*jvmti)->SetThreadLocalStorage(jvmti, thread, data);
        }
    }
    return data;
}

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbMethodEntry(jvmtiEnv* jvmti, JNIEnv* jni,
                                   jthread thread, jmethodID method) {
    thread_data_t* tdata = get_thread_data(jvmti, thread);
    if (!tdata) return;

    /* Reentrancy guard: skip if we're already inside a callback */
    if (tdata->reentrancy_guard) return;
    tdata->reentrancy_guard = 1;

    tdata->method_entry_count++;

    /* Only log every 100th method to avoid flooding */
    if (tdata->method_entry_count % 100 == 0) {
        char* class_sig = NULL;
        char* method_name = NULL;
        jclass declaring_class;

        if ((*jvmti)->GetMethodDeclaringClass(jvmti, method,
                                               &declaring_class) == JVMTI_ERROR_NONE) {
            (*jvmti)->GetClassSignature(jvmti, declaring_class,
                                         &class_sig, NULL);
        }
        (*jvmti)->GetMethodName(jvmti, method, &method_name, NULL, NULL);

        if (class_sig && method_name) {
            char buf[512];
            agent_lock_enter(jvmti);
            snprintf(buf, sizeof(buf),
                     "[Method #%d] %s.%s",
                     tdata->method_entry_count, class_sig, method_name);
            jvmti_log(LOG_FILE, buf);
            agent_lock_exit(jvmti);
        }

        if (class_sig) (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
        if (method_name) (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
    }

    tdata->reentrancy_guard = 0;
}

static void JNICALL cbThreadStart(jvmtiEnv* jvmti, JNIEnv* jni,
                                   jthread thread) {
    jvmtiThreadInfo tinfo;
    memset(&tinfo, 0, sizeof(tinfo));
    if ((*jvmti)->GetThreadInfo(jvmti, thread, &tinfo) == JVMTI_ERROR_NONE
        && tinfo.name) {
        char buf[256];
        agent_lock_enter(jvmti);
        snprintf(buf, sizeof(buf), "[ThreadStart] %s — TLS initialized",
                 tinfo.name);
        jvmti_log(LOG_FILE, buf);
        agent_lock_exit(jvmti);
        (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
    }
    /* Force TLS allocation for new thread */
    get_thread_data(jvmti, thread);
}

static void JNICALL cbThreadEnd(jvmtiEnv* jvmti, JNIEnv* jni,
                                 jthread thread) {
    /* Clean up TLS for this thread */
    thread_data_t* data = NULL;
    jvmtiError err = (*jvmti)->GetThreadLocalStorage(jvmti, thread,
                                                      (void**)&data);
    if (err == JVMTI_ERROR_NONE && data) {
        char buf[256];
        agent_lock_enter(jvmti);
        snprintf(buf, sizeof(buf),
                 "[ThreadEnd] TLS cleanup: %d method entries recorded",
                 data->method_entry_count);
        jvmti_log(LOG_FILE, buf);
        agent_lock_exit(jvmti);
        free(data);
        (*jvmti)->SetThreadLocalStorage(jvmti, thread, NULL);
    }
}

static void JNICALL cbVMDeath(jvmtiEnv* jvmti, JNIEnv* jni) {
    jvmti_log(LOG_FILE, "[Agent] VMDeath — agent shutting down.");

    /* Destroy the raw monitor last (book §9.7 best practices) */
    if (agent_lock) {
        (*jvmti)->DestroyRawMonitor(jvmti, agent_lock);
        agent_lock = NULL;
    }
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] advanced_agent loaded (Chapter 09)\n");

    jvmtiEnv* jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    jvmtiError err;

    /* Create raw monitor for thread-safe logging */
    err = (*jvmti)->CreateRawMonitor(jvmti, "agent_lock", &agent_lock);
    CHECK_JVMTI_ERROR(jvmti, err, "CreateRawMonitor");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Request capabilities */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_method_entry_events = 1;
    err = (*jvmti)->AddCapabilities(jvmti, &caps);
    CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.MethodEntry  = &cbMethodEntry;
    callbacks.ThreadStart  = &cbThreadStart;
    callbacks.ThreadEnd    = &cbThreadEnd;
    callbacks.VMDeath      = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Enable events */
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_METHOD_ENTRY, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable MethodEntry");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_THREAD_START, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable ThreadStart");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_THREAD_END, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable ThreadEnd");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");

    jvmti_log(LOG_FILE, "[Agent] Agent_OnLoad complete — advanced features active.");
    return JNI_OK;
}
