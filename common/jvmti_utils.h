/**
 * jvmti_utils.h — Shared utility macros and helpers for JVMTI agent examples.
 *
 * Provides:
 *   - jvmti_get_env()   : Acquire a jvmtiEnv* from a JavaVM*
 *   - CHECK_JVMTI_ERROR()     : Error-checking macro that returns JNI_ERR on failure
 *   - CHECK_JVMTI_ERROR_VOID(): Error-checking macro for void functions
 *   - jvmti_log()       : Simple append-to-file logger
 *
 * Each chapter can optionally #include this header to reduce boilerplate.
 * Chapter 01 intentionally does NOT use it, to show the raw JVMTI patterns.
 */

#ifndef JVMTI_UTILS_H
#define JVMTI_UTILS_H

#include <jvmti.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  JVMTI environment acquisition                                     */
/* ------------------------------------------------------------------ */

/**
 * Acquires a jvmtiEnv pointer from the given JavaVM.
 * Returns NULL on failure (with an error message printed to stderr).
 */
static inline jvmtiEnv* jvmti_get_env(JavaVM *vm) {
    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        fprintf(stderr, "[JVMTI] Unable to access JVMTI! (error=%d)\n", res);
        return NULL;
    }
    return jvmti;
}

/* ------------------------------------------------------------------ */
/*  Error-checking macros                                              */
/* ------------------------------------------------------------------ */

/**
 * Execute a JVMTI call and return JNI_ERR if it fails.
 * Use inside Agent_OnLoad or any function returning jint.
 *
 * Example:
 *   CHECK_JVMTI_ERROR((*jvmti)->AddCapabilities(jvmti, &caps), "Failed to add capabilities");
 */
#define CHECK_JVMTI_ERROR(expr, msg)                                     \
    do {                                                                 \
        jvmtiError _err = (expr);                                        \
        if (_err != JVMTI_ERROR_NONE) {                                  \
            fprintf(stderr, "[JVMTI] %s (error=%d)\n", (msg), _err);     \
            return JNI_ERR;                                              \
        }                                                                \
    } while (0)

/**
 * Execute a JVMTI call and return (void) if it fails.
 * Use inside callback functions that return void.
 */
#define CHECK_JVMTI_ERROR_VOID(expr, msg)                                \
    do {                                                                 \
        jvmtiError _err = (expr);                                        \
        if (_err != JVMTI_ERROR_NONE) {                                  \
            fprintf(stderr, "[JVMTI] %s (error=%d)\n", (msg), _err);     \
            return;                                                      \
        }                                                                \
    } while (0)

/* ------------------------------------------------------------------ */
/*  Simple file logger                                                 */
/* ------------------------------------------------------------------ */

/**
 * Appends a message line to the given log file.
 * Opens and closes the file on each call (safe for multi-agent use).
 */
static inline void jvmti_log(const char* filename, const char* msg) {
    FILE* fp = fopen(filename, "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

#endif /* JVMTI_UTILS_H */
