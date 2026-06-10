/**
 * jvmti_utils.h — Shared utility macros and helpers for JVMTI agent examples.
 *
 * Provides:
 *   - jvmti_get_env()     : Acquire a jvmtiEnv* from a JavaVM*
 *   - CHECK_JVMTI_ERROR() : Error-reporting macro (Chapter 2, §2.3)
 *   - jvmti_log()         : Simple append-to-file logger
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
        fprintf(stderr, "[JVMTI] Unable to access JVMTI! (error=%d)\n", (int)res);
        return NULL;
    }
    return jvmti;
}

/* ------------------------------------------------------------------ */
/*  Error-reporting macro (book Chapter 2, §2.3)                       */
/* ------------------------------------------------------------------ */

/**
 * Report a JVMTI error with its symbolic name, then continue.
 * Matches the macro introduced in Chapter 2 and used in every chapter.
 *
 * Example:
 *   jvmtiError err = (*jvmti)->AddCapabilities(jvmti, &caps);
 *   CHECK_JVMTI_ERROR(jvmti, err, "AddCapabilities");
 *   if (err != JVMTI_ERROR_NONE) return JNI_ERR;   // abort if critical
 */
#define CHECK_JVMTI_ERROR(jvmti, err, msg) \
    do { \
        if ((err) != JVMTI_ERROR_NONE) { \
            char *err_name = NULL; \
            (*jvmti)->GetErrorName((jvmti), (err), &err_name); \
            fprintf(stderr, "[Agent ERROR] %s: %s (%d)\n", \
                    (msg), err_name ? err_name : "unknown", (err)); \
            if (err_name) (*jvmti)->Deallocate((jvmti), (unsigned char*)err_name); \
        } \
    } while(0)

/* ------------------------------------------------------------------ */
/*  Simple file logger                                                 */
/* ------------------------------------------------------------------ */

/**
 * Appends a message line to the given log file.
 * Opens and closes the file on each call (safe for multi-agent use).
 *
 * NOTE: Do not call from GC or ObjectFree callbacks — those run in a
 * restricted environment where only simple bookkeeping is safe
 * (see Chapter 3, §3.7 and Chapter 7, §7.3).
 */
static inline void jvmti_log(const char* filename, const char* msg) {
    FILE* fp = fopen(filename, "a");
    if (fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

#endif /* JVMTI_UTILS_H */
