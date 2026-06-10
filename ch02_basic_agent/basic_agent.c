/**
 * Chapter 02 — Basic JVMTI Agent
 *
 * Demonstrates:
 *   - Agent_OnLoad / Agent_OnUnload lifecycle
 *   - Acquiring a jvmtiEnv pointer
 *   - Parsing agent options
 *   - Using CHECK_JVMTI_ERROR for robust error handling
 *   - Registering for VMInit and VMDeath events
 *
 * Build:
 *   mkdir build && cd build && cmake .. && cmake --build .
 *
 * Run:
 *   javac HelloApp.java
 *   java -agentpath:./build/libbasic_agent.so=verbose=true HelloApp
 */

#include "jvmti_utils.h"
#include <stdlib.h>

#define LOG_FILE "basic_agent.log"

/* Agent configuration parsed from options string */
static int verbose_mode = 0;

/* ------------------------------------------------------------------ */
/*  Option Parsing                                                     */
/* ------------------------------------------------------------------ */

/* Parse comma-separated key=value pairs (book §2.5), e.g.
   -agentpath:libbasic_agent.so=verbose=true,output=/tmp/trace.log */
static void parse_options(const char* options) {
    if (!options || strlen(options) == 0) return;

    char* opts = strdup(options);
    if (!opts) return;

    char* token = strtok(opts, ",");
    while (token) {
        char* eq = strchr(token, '=');
        if (eq != NULL) {
            *eq = '\0';
            const char* key = token;
            const char* value = eq + 1;
            printf("[Agent] Option: %s = %s\n", key, value);
            if (strcmp(key, "verbose") == 0 && strcmp(value, "true") == 0) {
                verbose_mode = 1;
                printf("[Agent] Verbose mode enabled\n");
            }
        }
        token = strtok(NULL, ",");
    }
    free(opts);
}

/* ------------------------------------------------------------------ */
/*  Event Callbacks                                                    */
/* ------------------------------------------------------------------ */

static void JNICALL cbVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread) {
    jvmti_log(LOG_FILE, "[Agent] VMInit event — JVM is fully initialized.");

    if (verbose_mode) {
        /* Print JVM version info */
        char* vm_version = NULL;
        jvmtiError err = (*jvmti)->GetSystemProperty(jvmti,
                             "java.vm.version", &vm_version);
        if (err == JVMTI_ERROR_NONE && vm_version) {
            char buf[256];
            snprintf(buf, sizeof(buf), "[Agent] JVM version: %s", vm_version);
            jvmti_log(LOG_FILE, buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)vm_version);
        }

        /* Print number of loaded classes */
        jint class_count = 0;
        jclass* classes = NULL;
        err = (*jvmti)->GetLoadedClasses(jvmti, &class_count, &classes);
        if (err == JVMTI_ERROR_NONE) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "[Agent] Loaded classes at VMInit: %d", (int)class_count);
            jvmti_log(LOG_FILE, buf);
            (*jvmti)->Deallocate(jvmti, (unsigned char*)classes);
        }
    }
}

static void JNICALL cbVMDeath(jvmtiEnv *jvmti, JNIEnv *jni) {
    jvmti_log(LOG_FILE, "[Agent] VMDeath event — JVM is shutting down.");
}

/* ------------------------------------------------------------------ */
/*  Agent Lifecycle                                                    */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Agent] basic_agent loaded (Chapter 02)\n");

    /* Parse agent options */
    parse_options(options);

    /* Acquire JVMTI environment */
    jvmtiEnv *jvmti = jvmti_get_env(vm);
    if (!jvmti) return JNI_ERR;

    /* Register event callbacks */
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit  = &cbVMInit;
    callbacks.VMDeath = &cbVMDeath;
    jvmtiError err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks,
                                                  sizeof(callbacks));
    CHECK_JVMTI_ERROR(jvmti, err, "SetEventCallbacks");
    if (err != JVMTI_ERROR_NONE) return JNI_ERR;

    /* Enable events */
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                             JVMTI_EVENT_VM_INIT, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMInit");
    err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                             JVMTI_EVENT_VM_DEATH, NULL);
    CHECK_JVMTI_ERROR(jvmti, err, "Enable VMDeath");

    jvmti_log(LOG_FILE, "[Agent] Agent_OnLoad complete — waiting for events.");
    return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
    printf("[Agent] basic_agent unloaded\n");
    jvmti_log(LOG_FILE, "[Agent] Agent_OnUnload — cleanup complete.");
}
