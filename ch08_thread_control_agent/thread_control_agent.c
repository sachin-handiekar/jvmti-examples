#include <jvmti.h>
#include <stdio.h>
#include <string.h>

static void log_msg(const char* msg) {
    FILE* fp = fopen("thread_control_agent.log", "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

static const char* thread_state_str(jint state) {
    static char buf[128];
    buf[0] = '\0';
    if (state & JVMTI_THREAD_STATE_ALIVE) strcat(buf, "ALIVE ");
    if (state & JVMTI_THREAD_STATE_TERMINATED) strcat(buf, "TERMINATED ");
    if (state & JVMTI_THREAD_STATE_RUNNABLE) strcat(buf, "RUNNABLE ");
    if (state & JVMTI_THREAD_STATE_BLOCKED_ON_MONITOR_ENTER) strcat(buf, "BLOCKED_ON_MONITOR_ENTER ");
    if (state & JVMTI_THREAD_STATE_WAITING) strcat(buf, "WAITING ");
    if (state & JVMTI_THREAD_STATE_WAITING_INDEFINITELY) strcat(buf, "WAITING_INDEFINITELY ");
    if (state & JVMTI_THREAD_STATE_WAITING_WITH_TIMEOUT) strcat(buf, "WAITING_WITH_TIMEOUT ");
    if (state & JVMTI_THREAD_STATE_SLEEPING) strcat(buf, "SLEEPING ");
    if (state & JVMTI_THREAD_STATE_IN_OBJECT_WAIT) strcat(buf, "IN_OBJECT_WAIT ");
    if (state & JVMTI_THREAD_STATE_PARKED) strcat(buf, "PARKED ");
    if (state & JVMTI_THREAD_STATE_SUSPENDED) strcat(buf, "SUSPENDED ");
    return buf;
}

static int is_system_thread(const char* name) {
    if (!name) return 1;
    return (
        strstr(name, "JNI Reference") ||
        strstr(name, "Finalizer") ||
        strstr(name, "Reference") ||
        strstr(name, "Signal Dispatcher") ||
        strstr(name, "VM Thread") ||
        strstr(name, "GC Thread") ||
        strstr(name, "Attach Listener") ||
        strstr(name, "CompilerThread")
    );
}

static void JNICALL cbVMInit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {
    jint thread_count = 0;
    jthread* threads = NULL;
    jvmtiError err = (*jvmti)->GetAllThreads(jvmti, &thread_count, &threads);
    if (err != JVMTI_ERROR_NONE) {
        log_msg("[JVMTI] Failed to get all threads.");
        return;
    }
    char buf[1024];
    for (int i = 0; i < thread_count; ++i) {
        jvmtiThreadInfo tinfo;
        memset(&tinfo, 0, sizeof(tinfo));
        err = (*jvmti)->GetThreadInfo(jvmti, threads[i], &tinfo);
        if (err != JVMTI_ERROR_NONE || is_system_thread(tinfo.name)) {
            if (tinfo.name) (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
            continue;
        }
        jint state = 0;
        (*jvmti)->GetThreadState(jvmti, threads[i], &state);
        snprintf(buf, sizeof(buf), "[Thread] %s BEFORE suspend: %s", tinfo.name, thread_state_str(state));
        log_msg(buf);
        err = (*jvmti)->SuspendThread(jvmti, threads[i]);
        if (err == JVMTI_ERROR_NONE) {
            (*jvmti)->GetThreadState(jvmti, threads[i], &state);
            snprintf(buf, sizeof(buf), "[Thread] %s AFTER suspend: %s", tinfo.name, thread_state_str(state));
            log_msg(buf);
            (*jvmti)->ResumeThread(jvmti, threads[i]);
            (*jvmti)->GetThreadState(jvmti, threads[i], &state);
            snprintf(buf, sizeof(buf), "[Thread] %s AFTER resume: %s", tinfo.name, thread_state_str(state));
            log_msg(buf);
        } else {
            snprintf(buf, sizeof(buf), "[Thread] %s: Failed to suspend (err=%d)", tinfo.name, err);
            log_msg(buf);
        }
        if (tinfo.name) (*jvmti)->Deallocate(jvmti, (unsigned char*)tinfo.name);
    }
    if (threads) (*jvmti)->Deallocate(jvmti, (unsigned char*)threads);
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
    printf("[JVMTI] Thread control agent loaded and VMInit callback registered.\n");
    return JNI_OK;
}
