/**
 * Chapter 10 — Minimal Sampling Profiler
 *
 * Demonstrates:
 *   - A dedicated sampling thread started with RunAgentThread
 *   - Periodic GetStackTrace snapshots of all live threads (every 10ms)
 *   - Aggregating identical stacks into collapsed-stack format
 *   - Writing flamegraph-compatible output at VMDeath
 *
 * Sampling (snapshot stacks at intervals) is preferred over
 * instrumentation (hooking every method entry/exit) for CPU profiling:
 * instrumentation fires for millions of calls per second and distorts
 * the very timings it measures (book §10.1).
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac ProfilerTestApp.java
 *   java -agentpath:./build/libprofiler_agent.so ProfilerTestApp
 *
 * Visualize:
 *   git clone https://github.com/brendangregg/FlameGraph.git
 *   cat profiler_output.txt | ./FlameGraph/flamegraph.pl > profile.svg
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>   /* Sleep() */
#else
#include <time.h>      /* nanosleep() */
#endif

/* ── Configuration ── */
#define MAX_FRAMES     64
#define MAX_SAMPLES    10000
#define SAMPLE_KEY_LEN 2048

/* ── Sample storage ── */
typedef struct {
    char key[SAMPLE_KEY_LEN];  /* semicolon-separated stack */
    int  count;
} StackSample;

static StackSample samples[MAX_SAMPLES];
static int sample_count = 0;
static jrawMonitorID profiler_lock;
static volatile int profiling_active = 0;

static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
#endif
}

/* ── Record or increment a stack sample ── */
static void record_sample(const char *key) {
    /* Linear search — adequate for a minimal profiler */
    for (int i = 0; i < sample_count; i++) {
        if (strcmp(samples[i].key, key) == 0) {
            samples[i].count++;
            return;
        }
    }
    if (sample_count < MAX_SAMPLES) {
        strncpy(samples[sample_count].key, key, SAMPLE_KEY_LEN - 1);
        samples[sample_count].key[SAMPLE_KEY_LEN - 1] = '\0';
        samples[sample_count].count = 1;
        sample_count++;
    }
}

/* ── Sample one thread's stack ── */
static void sample_one_thread(jvmtiEnv *jvmti, jthread thread) {
    jvmtiFrameInfo frames[MAX_FRAMES];
    jint frame_count;

    jvmtiError err = (*jvmti)->GetStackTrace(jvmti, thread, 0,
                                              MAX_FRAMES, frames, &frame_count);
    if (err != JVMTI_ERROR_NONE || frame_count == 0) return;

    /* Build the collapsed stack key (bottom → top) */
    char key[SAMPLE_KEY_LEN];
    key[0] = '\0';
    int pos = 0;

    for (int i = frame_count - 1; i >= 0; i--) {
        char *name = NULL, *csig = NULL;
        jclass dcls;

        (*jvmti)->GetMethodName(jvmti, frames[i].method, &name, NULL, NULL);
        if ((*jvmti)->GetMethodDeclaringClass(jvmti, frames[i].method,
                                               &dcls) == JVMTI_ERROR_NONE) {
            (*jvmti)->GetClassSignature(jvmti, dcls, &csig, NULL);
        }

        /* Format: Lcom/foo/Bar; → com/foo/Bar
           Strip BOTH the leading 'L' and the trailing ';' BEFORE
           formatting. The ';' is the frame separator in collapsed
           stack format — leaving it embedded in the key would make
           flamegraph.pl split every frame at the wrong place. */
        const char *class_name = "?";
        if (csig) {
            size_t slen = strlen(csig);
            if (csig[0] == 'L' && slen > 1) {
                if (csig[slen - 1] == ';')
                    csig[slen - 1] = '\0';   /* drop trailing ';' */
                class_name = csig + 1;        /* drop leading 'L'  */
            } else {
                class_name = csig;            /* array/primitive sig */
            }
        }

        int remaining = SAMPLE_KEY_LEN - pos;
        if (remaining <= 1) {  /* key buffer full — truncate stack */
            if (name) (*jvmti)->Deallocate(jvmti, (unsigned char*)name);
            if (csig) (*jvmti)->Deallocate(jvmti, (unsigned char*)csig);
            break;
        }
        int len = snprintf(key + pos, (size_t)remaining,
                           "%s%s.%s",
                           (pos > 0) ? ";" : "",
                           class_name,
                           name ? name : "?");
        if (len > 0)
            pos += (len < remaining) ? len : remaining - 1;

        if (name) (*jvmti)->Deallocate(jvmti, (unsigned char*)name);
        if (csig) (*jvmti)->Deallocate(jvmti, (unsigned char*)csig);
    }

    if (pos > 0) {
        (*jvmti)->RawMonitorEnter(jvmti, profiler_lock);
        record_sample(key);
        (*jvmti)->RawMonitorExit(jvmti, profiler_lock);
    }
}

/* ── Sampling loop (runs on a dedicated agent thread) ── */
static void JNICALL sample_thread_func(jvmtiEnv *jvmti, JNIEnv *jni, void *arg) {
    printf("[Profiler] Sampling thread started\n");

    while (profiling_active) {
        jthread *threads = NULL;
        jint thread_count;

        jvmtiError err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
        if (err == JVMTI_ERROR_NONE) {
            for (int i = 0; i < thread_count; i++) {
                sample_one_thread(jvmti, threads[i]);
            }
            (*jvmti)->Deallocate(jvmti, (unsigned char*)threads);
        }

        sleep_ms(10);   /* ~10ms between samples */
    }
    printf("[Profiler] Sampling thread stopped\n");
}

/* ── Write flamegraph-format output ── */
static void write_output(void) {
    FILE *f = fopen("profiler_output.txt", "w");
    if (!f) { fprintf(stderr, "[Profiler] Cannot open output file\n"); return; }

    for (int i = 0; i < sample_count; i++) {
        fprintf(f, "%s %d\n", samples[i].key, samples[i].count);
    }
    fclose(f);
    printf("[Profiler] Wrote %d unique stacks to profiler_output.txt\n", sample_count);
}

/* ── Callbacks ── */
static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    profiling_active = 1;

    /* RunAgentThread requires a real (unstarted) java.lang.Thread
       object — passing NULL fails with JVMTI_ERROR_INVALID_THREAD.
       Create one via JNI (safe here: VM_INIT means full JNI is live). */
    jclass thread_class = (*jni)->FindClass(jni, "java/lang/Thread");
    jmethodID ctor = (*jni)->GetMethodID(jni, thread_class, "<init>", "()V");
    jthread agent_thread = (*jni)->NewObject(jni, thread_class, ctor);
    if (agent_thread == NULL) {
        (*jni)->ExceptionClear(jni);
        fprintf(stderr, "[Profiler] Failed to create agent thread object\n");
        profiling_active = 0;
        return;
    }

    jvmtiError err = (*jvmti)->RunAgentThread(jvmti, agent_thread,
        sample_thread_func, NULL, JVMTI_THREAD_NORM_PRIORITY);
    CHECK_JVMTI_ERROR(jvmti, err, "RunAgentThread");
    if (err != JVMTI_ERROR_NONE) profiling_active = 0;
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    profiling_active = 0;
    sleep_ms(50);   /* give the sampling thread time to stop */

    (*jvmti)->RawMonitorEnter(jvmti, profiler_lock);
    write_output();
    (*jvmti)->RawMonitorExit(jvmti, profiler_lock);

    (*jvmti)->DestroyRawMonitor(jvmti, profiler_lock);
}

/* ── Agent entry point ── */
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiEnv *jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    jvmtiError err = (*jvmti)->CreateRawMonitor(jvmti, "ProfilerLock", &profiler_lock);
    CHECK_JVMTI_ERROR(jvmti, err, "CreateRawMonitor");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit  = &cbVMInit;
    callbacks.VMDeath = &cbVMDeath;
    err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_INIT, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMInit");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
              JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");

    printf("[Profiler] Sampling profiler agent loaded (interval: 10ms)\n");
    return JNI_OK;
}
