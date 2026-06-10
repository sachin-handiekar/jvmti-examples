/**
 * Chapter 11 — Deploying and Testing JVMTI Agents
 *
 * A configurable, production-style agent demonstrating:
 *   - Both entry points: Agent_OnLoad (startup) and Agent_OnAttach
 *     (live attach via jcmd / the Attach API, book §11.4)
 *   - Option parsing: package=<prefix>,method_entry,method_exit,thread_events
 *   - Thread-safe file logging with rotation (raw-monitor protected —
 *     callbacks fire concurrently on every Java thread)
 *   - Graceful degradation when a capability is unavailable at attach
 *     time (GetPotentialCapabilities)
 *
 * Load at startup:
 *   java -agentpath:./build/libprod_agent.so=package=com/example,method_entry HelloService
 *
 * Attach to a running JVM (Java 9+):
 *   jcmd <pid> JVMTI.agent_load /abs/path/libprod_agent.so thread_events
 */

#include <jvmti.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AGENT_VERSION "1.1.0"

#define LOG_FILE_BASE "prod_agent.log"
#define LOG_FILE_MAX_SIZE (1024 * 10) /* 10 KB for demo */
#define LOG_FILE_ROTATE_COUNT 3

static FILE* log_fp = NULL;
static long  log_bytes_written = 0;
static jrawMonitorID log_lock = NULL;

static char monitored_package[256] = "com/example";
static int enable_method_entry = 0;
static int enable_method_exit = 0;
static int enable_thread_events = 0;

/* ------------------------------------------------------------------ */
/*  Thread-safe logging with rotation                                  */
/* ------------------------------------------------------------------ */

/* Must be called with log_lock held */
static void rotate_logs_locked(void) {
    if (log_fp) { fclose(log_fp); log_fp = NULL; }

    /* Remove oldest log if it exists, then shift the rest up */
    char oldlog[64];
    snprintf(oldlog, sizeof(oldlog), LOG_FILE_BASE ".%d", LOG_FILE_ROTATE_COUNT);
    remove(oldlog);
    for (int i = LOG_FILE_ROTATE_COUNT - 1; i >= 1; --i) {
        char src[64], dst[64];
        snprintf(src, sizeof(src), LOG_FILE_BASE ".%d", i);
        snprintf(dst, sizeof(dst), LOG_FILE_BASE ".%d", i + 1);
        rename(src, dst);
    }
    rename(LOG_FILE_BASE, LOG_FILE_BASE ".1");

    log_fp = fopen(LOG_FILE_BASE, "w");
    log_bytes_written = 0;
}

static void log_msg(jvmtiEnv* jvmti, const char* msg) {
    if (log_lock) (*jvmti)->RawMonitorEnter(jvmti, log_lock);

    if (!log_fp) {
        log_fp = fopen(LOG_FILE_BASE, "a");
        log_bytes_written = 0;
    }
    if (log_fp) {
        fprintf(log_fp, "%s\n", msg);
        fflush(log_fp);
        log_bytes_written += (long)strlen(msg) + 1;
        if (log_bytes_written > LOG_FILE_MAX_SIZE) {
            rotate_logs_locked();
        }
    }

    if (log_lock) (*jvmti)->RawMonitorExit(jvmti, log_lock);
}

/* ------------------------------------------------------------------ */
/*  Option Parsing                                                     */
/* ------------------------------------------------------------------ */

static void parse_options(const char* options) {
    if (!options) return;
    char* opts = strdup(options);
    if (!opts) return;

    char* token = strtok(opts, ",");
    while (token) {
        if (strncmp(token, "package=", 8) == 0) {
            strncpy(monitored_package, token + 8, sizeof(monitored_package) - 1);
            monitored_package[sizeof(monitored_package) - 1] = '\0';
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

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void log_method_event(jvmtiEnv* jvmti, jmethodID method,
                              const char* tag) {
    char* class_sig = NULL;
    jclass declaring_class;
    if ((*jvmti)->GetMethodDeclaringClass(jvmti, method,
                                           &declaring_class) != JVMTI_ERROR_NONE)
        return;
    if ((*jvmti)->GetClassSignature(jvmti, declaring_class,
                                     &class_sig, NULL) != JVMTI_ERROR_NONE)
        return;

    if (class_sig[0] == 'L' && strstr(class_sig, monitored_package)) {
        char* method_name = NULL;
        char* method_sig = NULL;
        if ((*jvmti)->GetMethodName(jvmti, method, &method_name,
                                     &method_sig, NULL) == JVMTI_ERROR_NONE) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s %s.%s%s",
                     tag, class_sig, method_name, method_sig);
            log_msg(jvmti, buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)method_name);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)method_sig);
        }
    }
    (*jvmti)->Deallocate(jvmti, (unsigned char*)class_sig);
}

static void JNICALL cbMethodEntry(jvmtiEnv* jvmti, JNIEnv* jni,
                                   jthread thread, jmethodID method) {
    log_method_event(jvmti, method, "[Entry]");
}

static void JNICALL cbMethodExit(jvmtiEnv* jvmti, JNIEnv* jni,
                                  jthread thread, jmethodID method,
                                  jboolean was_popped_by_exception,
                                  jvalue return_value) {
    log_method_event(jvmti, method, "[Exit] ");
}

static void log_thread_event(jvmtiEnv* jvmti, jthread thread, const char* tag) {
    jvmtiThreadInfo tinfo;
    memset(&tinfo, 0, sizeof(tinfo));
    if ((*jvmti)->GetThreadInfo(jvmti, thread, &tinfo) == JVMTI_ERROR_NONE
        && tinfo.name) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s %s", tag, tinfo.name);
        log_msg(jvmti, buf);
        (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
    }
}

static void JNICALL cbThreadStart(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    log_thread_event(jvmti, thread, "[ThreadStart]");
}

static void JNICALL cbThreadEnd(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    log_thread_event(jvmti, thread, "[ThreadEnd]");
}

/* ------------------------------------------------------------------ */
/*  Shared initialization (used by both entry points, §11.6)           */
/* ------------------------------------------------------------------ */

static jint agent_init(JavaVM *vm, char *options, int attached) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        fprintf(stderr, "[JVMTI] Unable to access JVMTI!\n");
        return JNI_ERR;
    }

    parse_options(options);

    jvmtiError err = (*jvmti)->CreateRawMonitor(jvmti, "prod_log_lock",
                                                 &log_lock);
    if (err != JVMTI_ERROR_NONE) {
        fprintf(stderr, "[JVMTI] Failed to create raw monitor: %d\n", err);
        return JNI_ERR;
    }

    /* Not all capabilities are available after the JVM has started
       (§11.4) — check what this JVM offers right now and degrade. */
    jvmtiCapabilities potential;
    memset(&potential, 0, sizeof(potential));
    (*jvmti)->GetPotentialCapabilities(jvmti, &potential);

    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    if (enable_method_entry) {
        if (potential.can_generate_method_entry_events) {
            caps.can_generate_method_entry_events = 1;
        } else {
            log_msg(jvmti, "[JVMTI] method_entry unavailable on this JVM/phase — disabled.");
            enable_method_entry = 0;
        }
    }
    if (enable_method_exit) {
        if (potential.can_generate_method_exit_events) {
            caps.can_generate_method_exit_events = 1;
        } else {
            log_msg(jvmti, "[JVMTI] method_exit unavailable on this JVM/phase — disabled.");
            enable_method_exit = 0;
        }
    }
    /* ThreadStart/ThreadEnd events do not require a special capability */

    err = (*jvmti)->AddCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        log_msg(jvmti, "[JVMTI] Failed to add capabilities.");
        return JNI_ERR;
    }

    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    if (enable_method_entry) callbacks.MethodEntry = &cbMethodEntry;
    if (enable_method_exit)  callbacks.MethodExit  = &cbMethodExit;
    if (enable_thread_events) {
        callbacks.ThreadStart = &cbThreadStart;
        callbacks.ThreadEnd   = &cbThreadEnd;
    }
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    if (err != JVMTI_ERROR_NONE) {
        log_msg(jvmti, "[JVMTI] Failed to set event callbacks.");
        return JNI_ERR;
    }

    if (enable_method_entry) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                  JVMTI_EVENT_METHOD_ENTRY, NULL);
        if (err != JVMTI_ERROR_NONE)
            log_msg(jvmti, "[JVMTI] Failed to enable METHOD_ENTRY event.");
    }
    if (enable_method_exit) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                  JVMTI_EVENT_METHOD_EXIT, NULL);
        if (err != JVMTI_ERROR_NONE)
            log_msg(jvmti, "[JVMTI] Failed to enable METHOD_EXIT event.");
    }
    if (enable_thread_events) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                  JVMTI_EVENT_THREAD_START, NULL);
        if (err != JVMTI_ERROR_NONE)
            log_msg(jvmti, "[JVMTI] Failed to enable THREAD_START event.");
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                  JVMTI_EVENT_THREAD_END, NULL);
        if (err != JVMTI_ERROR_NONE)
            log_msg(jvmti, "[JVMTI] Failed to enable THREAD_END event.");
    }

    char buf[512];
    snprintf(buf, sizeof(buf),
             "[JVMTI] Production agent v%s %s (package=%s entry=%d exit=%d threads=%d)",
             AGENT_VERSION, attached ? "attached to running JVM" : "loaded at startup",
             monitored_package, enable_method_entry, enable_method_exit,
             enable_thread_events);
    log_msg(jvmti, buf);
    return JNI_OK;
}

/* ------------------------------------------------------------------ */
/*  Entry Points                                                       */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    return agent_init(vm, options, 0);
}

JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {
    return agent_init(vm, options, 1);
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
    if (log_fp) {
        fprintf(log_fp, "[JVMTI] Agent unloading, closing log file.\n");
        fclose(log_fp);
        log_fp = NULL;
    }
}
