/**
 * Chapter 04 — Thread, Stack, and Method Inspection Agent
 *
 * Produces a Java-style thread dump at VMInit (book §4.5):
 *   - GetAllThreads / GetThreadInfo for thread enumeration and metadata
 *   - GetThreadState with correct flag ordering (SLEEPING is a sub-state
 *     of WAITING, so the more specific flag is tested first)
 *   - GetStackTrace + GetMethodName / GetMethodDeclaringClass /
 *     GetClassSignature to resolve each frame
 *   - GetLineNumberTable (capability: can_get_line_numbers) to map
 *     bytecode indices to source lines when debug info is present
 *
 * Also logs the loaded-class count as a bonus (GetLoadedClasses).
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac -g InspectionTestApp.java     # -g enables line numbers
 *   java -agentpath:./build/libthread_class_inspection_agent.so InspectionTestApp
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE   "thread_class_inspection.log"
#define MAX_FRAMES 64

/* ------------------------------------------------------------------ */
/*  Thread dump                                                        */
/* ------------------------------------------------------------------ */

static const char* thread_state_name(jint state) {
    if (state & JVMTI_THREAD_STATE_RUNNABLE) return "RUNNABLE";
    if (state & JVMTI_THREAD_STATE_BLOCKED_ON_MONITOR_ENTER) return "BLOCKED";
    /* SLEEPING implies WAITING — test the specific flag first (§4.1) */
    if (state & JVMTI_THREAD_STATE_SLEEPING) return "TIMED_WAITING";
    if (state & JVMTI_THREAD_STATE_WAITING) return "WAITING";
    return "UNKNOWN";
}

static void dump_thread_stack(jvmtiEnv *jvmti, jthread thread) {
    char buf[1024];

    jvmtiThreadInfo tinfo;
    memset(&tinfo, 0, sizeof(tinfo));
    jvmtiError err = (*jvmti)->GetThreadInfo(jvmti, thread, &tinfo);
    if (err != JVMTI_ERROR_NONE) return;

    jint state = 0;
    (*jvmti)->GetThreadState(jvmti, thread, &state);

    snprintf(buf, sizeof(buf), "\"%s\" %s priority=%d daemon=%s",
             tinfo.name ? tinfo.name : "<unnamed>",
             thread_state_name(state), (int)tinfo.priority,
             tinfo.is_daemon ? "true" : "false");
    jvmti_log(LOG_FILE, buf);

    /* Walk the stack */
    jvmtiFrameInfo frames[MAX_FRAMES];
    jint frame_count = 0;
    err = (*jvmti)->GetStackTrace(jvmti, thread, 0, MAX_FRAMES,
                                   frames, &frame_count);
    if (err == JVMTI_ERROR_NONE) {
        for (int j = 0; j < frame_count; j++) {
            char *method_name = NULL, *method_sig = NULL, *class_sig = NULL;
            jclass declaring_class;

            (*jvmti)->GetMethodName(jvmti, frames[j].method,
                                     &method_name, &method_sig, NULL);
            err = (*jvmti)->GetMethodDeclaringClass(jvmti, frames[j].method,
                                                     &declaring_class);
            if (err == JVMTI_ERROR_NONE) {
                (*jvmti)->GetClassSignature(jvmti, declaring_class,
                                             &class_sig, NULL);
            }

            /* Try to map the bytecode index to a source line
               (requires can_get_line_numbers + javac -g) */
            jint line = -1;
            jint line_count;
            jvmtiLineNumberEntry *line_table;
            err = (*jvmti)->GetLineNumberTable(jvmti, frames[j].method,
                                                &line_count, &line_table);
            if (err == JVMTI_ERROR_NONE) {
                for (int k = 0; k < line_count; k++) {
                    if (line_table[k].start_location <= frames[j].location)
                        line = line_table[k].line_number;
                }
                (*jvmti)->Deallocate(jvmti, (unsigned char*)line_table);
            }

            if (line >= 0)
                snprintf(buf, sizeof(buf), "    at %s.%s%s (line %d)",
                         class_sig ? class_sig : "?",
                         method_name ? method_name : "?",
                         method_sig ? method_sig : "", (int)line);
            else
                snprintf(buf, sizeof(buf), "    at %s.%s%s",
                         class_sig ? class_sig : "?",
                         method_name ? method_name : "?",
                         method_sig ? method_sig : "");
            jvmti_log(LOG_FILE, buf);

            if (method_name) (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
            if (method_sig)  (*jvmti)->Deallocate(jvmti, (unsigned char*)method_sig);
            if (class_sig)   (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
        }
    }
    jvmti_log(LOG_FILE, "");

    if (tinfo.name) (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
}

static void dump_all_threads(jvmtiEnv *jvmti) {
    jint thread_count = 0;
    jthread *threads = NULL;

    jvmtiError err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
    CHECK_JVMTI_ERROR(jvmti, err, "GetAllThreads");
    if (err != JVMTI_ERROR_NONE) return;

    char buf[256];
    snprintf(buf, sizeof(buf),
             "========== JVMTI Thread Dump (%d threads) ==========",
             (int)thread_count);
    jvmti_log(LOG_FILE, buf);
    jvmti_log(LOG_FILE, "");

    for (int i = 0; i < thread_count; i++) {
        dump_thread_stack(jvmti, threads[i]);
    }

    jvmti_log(LOG_FILE, "====================================================");

    (*jvmti)->Deallocate(jvmti, (unsigned char*)threads);
}

/* ------------------------------------------------------------------ */
/*  Loaded class count (bonus)                                         */
/* ------------------------------------------------------------------ */

static void log_loaded_class_count(jvmtiEnv *jvmti) {
    jint class_count = 0;
    jclass *classes = NULL;
    jvmtiError err = (*jvmti)->GetLoadedClasses(jvmti, &class_count, &classes);
    if (err != JVMTI_ERROR_NONE) return;

    char buf[256];
    snprintf(buf, sizeof(buf), "[Inspect] Loaded classes at VMInit: %d",
             (int)class_count);
    jvmti_log(LOG_FILE, buf);
    (*jvmti)->Deallocate(jvmti, (unsigned char*)classes);
}

/* ------------------------------------------------------------------ */
/*  Callbacks and Agent Lifecycle                                      */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmti_log(LOG_FILE, "[Inspect] VMInit — dumping threads...");
    log_loaded_class_count(jvmti);
    dump_all_threads(jvmti);
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] thread_class_inspection_agent loaded (Chapter 04)\n");

    jvmtiEnv *jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    /* Request line-number capability for source line resolution */
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_get_line_numbers = 1;
    jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
    CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");

    /* Register VMInit callback — dump threads once the JVM is ready */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &cbVMInit;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_INIT, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMInit");

    printf("[Agent] Thread dump agent ready — output in %s\n", LOG_FILE);
    return JNI_OK;
}
