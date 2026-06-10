/**
 * Chapter 12 — Security and Stability Agent
 *
 * Demonstrates the defensive patterns that separate prototype agents
 * from production-ready ones:
 *
 *   - Defensive capability checking with GetPotentialCapabilities and
 *     graceful degradation when a feature is unavailable (§12.4)
 *   - The safe-shutdown pattern: a shutting_down flag, disabling events
 *     BEFORE cleanup, and destroying raw monitors last (§12.2)
 *   - Crash-handler chaining (§12.1): the JVM deliberately uses SIGSEGV
 *     for implicit null checks and safepoint polls, so an agent must
 *     CHAIN to the JVM's handler, never replace it. On Windows, the SEH
 *     filter returns EXCEPTION_CONTINUE_SEARCH for the same reason.
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac StabilityTestApp.java
 *   java -agentpath:./build/libstability_agent.so StabilityTestApp
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE "stability_agent.log"

/* ------------------------------------------------------------------ */
/*  Shared state (protected by agent_lock; see §12.2 thread rules)     */
/* ------------------------------------------------------------------ */

static jrawMonitorID agent_lock = NULL;
static volatile int shutting_down = 0;
static int exception_monitoring = 0;   /* degraded gracefully if unsupported */
static long exception_count = 0;

/* ------------------------------------------------------------------ */
/*  Crash handler chaining (§12.1)                                     */
/*                                                                     */
/*  Most agents shouldn't install fault handlers at all — the JVM's    */
/*  hs_err_pid*.log already captures everything, including agent       */
/*  frames. This demo installs one only to show CORRECT chaining.      */
/*  The simplest production option is libjsig:                         */
/*    LD_PRELOAD=$JAVA_HOME/lib/libjsig.so java ...                    */
/* ------------------------------------------------------------------ */

#ifdef _WIN32

#include <windows.h>

static LONG WINAPI agent_exception_filter(EXCEPTION_POINTERS *ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    fprintf(stderr, "[Agent] SEH exception: 0x%08lx\n", (unsigned long)code);
    /* Let the JVM (and any other handlers) process it — never swallow */
    return EXCEPTION_CONTINUE_SEARCH;
}

static void install_crash_handlers(void) {
    SetUnhandledExceptionFilter(agent_exception_filter);
}

#else /* POSIX */

#include <signal.h>
#include <unistd.h>

static struct sigaction prev_segv_action;

static void crash_handler(int sig, siginfo_t *info, void *context) {
    /* ONLY async-signal-safe functions allowed here:
       no printf, no malloc, no JVMTI, no JNI. */
    const char *msg = "[Agent] signal received in agent code\n";
    write(STDERR_FILENO, msg, strlen(msg));

    /* CHAIN to the JVM's handler — it decides whether this fault is
       a normal implicit null check / safepoint poll (recover) or a
       real crash (write hs_err log and abort). NEVER _exit() here. */
    if (prev_segv_action.sa_flags & SA_SIGINFO) {
        prev_segv_action.sa_sigaction(sig, info, context);
    } else if (prev_segv_action.sa_handler != SIG_IGN &&
               prev_segv_action.sa_handler != SIG_DFL) {
        prev_segv_action.sa_handler(sig);
    } else {
        /* No previous handler: restore default and re-raise */
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

static void install_crash_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    /* Save the JVM's existing handler so we can chain to it */
    sigaction(SIGSEGV, &sa, &prev_segv_action);
}

#endif /* _WIN32 */

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbException(jvmtiEnv *jvmti, JNIEnv *jni,
                                 jthread thread, jmethodID method,
                                 jlocation location, jobject exception,
                                 jmethodID catch_method,
                                 jlocation catch_location) {
    if (shutting_down) return;   /* skip work during shutdown (§12.2) */

    (*jvmti)->RawMonitorEnter(jvmti, agent_lock);
    exception_count++;
    (*jvmti)->RawMonitorExit(jvmti, agent_lock);
}

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    /* Install crash handlers AFTER the JVM has installed its own,
       so the saved "previous" handler is the JVM's (§12.1). */
    install_crash_handlers();
    jvmti_log(LOG_FILE, "[Stability] VMInit — chained crash handlers installed.");
}

/* Safe shutdown pattern (§12.2): flag, disable events, then clean up */
static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    shutting_down = 1;

    /* Disable all events FIRST so no callback runs during teardown */
    if (exception_monitoring) {
        (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_DISABLE,
            JVMTI_EVENT_EXCEPTION, NULL);
    }

    /* Now safe to flush data */
    (*jvmti)->RawMonitorEnter(jvmti, agent_lock);
    char buf[256];
    if (exception_monitoring) {
        snprintf(buf, sizeof(buf),
                 "[Stability] VMDeath — %ld exceptions observed.",
                 exception_count);
    } else {
        snprintf(buf, sizeof(buf),
                 "[Stability] VMDeath — exception monitoring was disabled "
                 "(capability unavailable).");
    }
    jvmti_log(LOG_FILE, buf);
    printf("%s\n", buf);
    (*jvmti)->RawMonitorExit(jvmti, agent_lock);

    /* Destroy monitors last */
    (*jvmti)->DestroyRawMonitor(jvmti, agent_lock);
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] stability_agent loaded (Chapter 12)\n");

    jvmtiEnv *jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    jvmtiError err;

    err = (*jvmti)->CreateRawMonitor(jvmti, "stability_lock", &agent_lock);
    CHECK_JVMTI_ERROR(jvmti, err, "CreateRawMonitor");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Defensive capability check (§12.4): ask what this JVM actually
       supports and degrade gracefully instead of failing to load. */
    jvmtiCapabilities potential;
    memset(&potential, 0, sizeof(potential));
    err = (*jvmti)->GetPotentialCapabilities(jvmti, &potential);
    CHECK_JVMTI_ERROR(jvmti, err, "GetPotentialCapabilities");

    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    if (potential.can_generate_exception_events) {
        caps.can_generate_exception_events = 1;
        exception_monitoring = 1;
    } else {
        fprintf(stderr, "[Agent] WARNING: Exception events not supported "
                        "on this JVM — feature disabled\n");
    }

    err = (*jvmti)->AddCapabilities(jvmti, &caps);
    CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");

    /* Register callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit  = &cbVMInit;
    callbacks.VMDeath = &cbVMDeath;
    if (exception_monitoring) callbacks.Exception = &cbException;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Enable events */
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_INIT, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMInit");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");
    if (exception_monitoring) {
        err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                  JVMTI_EVENT_EXCEPTION, NULL);
        CHECK_JVMTI_ERROR(jvmti, err, "Enable Exception");
    }

    jvmti_log(LOG_FILE, "[Stability] Agent loaded — defensive mode active.");
    return JNI_OK;
}
