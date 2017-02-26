#include "jvmti.h"

#include "include/library.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <cstdarg>

#include <iostream>
#include <jvmti.h>

using namespace std;

/**
 * Fatal Error
 *
 * @param format
 * @param ...
 */
void fatalError(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    (void) printf(format, ap);
    va_end(ap);
    exit(3);
}


/**
 * Check JVMTI Error
 *
 * @param jvmti
 * @param errNum
 * @param str
 */
static bool checkJVMTIError(jvmtiEnv *jvmti, jvmtiError errNum, const char *str) {


    if (errNum != JVMTI_ERROR_NONE) {
        char *errnum_str;

        errnum_str = NULL;


        (void) jvmti->GetErrorName(errNum, &errnum_str);


        printf("JVMTI: ERROR %d %s: %s \n", errNum,
               (errnum_str == NULL ? "Unknown error" : errnum_str), (str == NULL ? "" : str));

        return false;

    }

    return true;
}


static void JNICALL
callbackExceptionEvent(jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jmethodID method, jlocation location,
                       jobject exception, jmethodID catchMethod, jlocation catchLocation) {

    jvmtiFrameInfo frames[10];
    jint count, entryCountPtr;
    jvmtiError error;
    char *sig, *gsig;
    jclass declaringClazz;
    jvmtiLocalVariableEntry *localVariableEntry;

    jvmtiPhase jvmtiPhasePtr;
    error = jvmti->GetPhase(&jvmtiPhasePtr);

    if (jvmtiPhasePtr != JVMTI_PHASE_LIVE) {
        return;
    }

    error = jvmti->GetStackTrace(thread, 0, 5, frames, &count);

    if (checkJVMTIError(jvmti, error, "Cannot Get Frame") && count >= 1) {
        char *methodName, *className;

        for (int i = 0; i < count; ++i) {

            error = jvmti->GetMethodName(frames[i].method, &methodName, &sig, &gsig);
            checkJVMTIError(jvmti, error, "Cannot get Method Name");


            error = jvmti->GetMethodDeclaringClass(frames[i].method, &declaringClazz);
            checkJVMTIError(jvmti, error, "Cannot get Method Declaring Class");


            error = jvmti->GetClassSignature(declaringClazz, &className, NULL);
            checkJVMTIError(jvmti, error, "Cannot get Class Signature");



            /**
             * Skip classes with java/lang, java/net and sun/reflect
             * TODO : Add more generic classes
             */
            if (strstr(className, "java/lang") != NULL
                || strstr(className, "java/net") != NULL
                || strstr(className, "sun/reflect") != NULL
                || strstr(className, "com/intellij/rt") != NULL) {
                break;
            }


            printf("\n Class Name : %s", className);


            error = jvmti->GetLocalVariableTable(frames[i].method, &entryCountPtr, &localVariableEntry);
            checkJVMTIError(jvmti, error, "Cannot Get Local Variable Table");

            if (error) {
                return;
            }


            printf("\n Got Exception in Method : %s", methodName);
            /*
            printf("\nGot exception in Method : %s,"
                           " Line : %lld "
                           "with Signature : %s , "
                           "Generic Signature : %s , "
                           "ClassName : %s ",
                   methodName,
                   frames[i].location,
                   sig,
                   gsig,
                   className);*/


        }
    }
}


JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {


    jvmtiEnv *jvmti;
    jvmtiCapabilities capabilities;

    jvmtiError error;
    jint result;

    jvmtiEventCallbacks eventCallbacks;
    jrawMonitorID rawMonitorID;


    result = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION);

    if (result != JNI_OK) {
        printf("\n Unable to access JVMTI!!!");
    }


    (void) memset(&capabilities, 0, sizeof(jvmtiCapabilities));

    capabilities.can_generate_exception_events = 1;
    capabilities.can_access_local_variables = 1;
    capabilities.can_get_constant_pool = 1;
    capabilities.can_get_bytecodes = 1;


    error = jvmti->AddCapabilities(&capabilities);
    checkJVMTIError(jvmti, error, "Unable to set Capabilities");

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread) NULL);
    checkJVMTIError(jvmti, error, "Cannot set Exception Event Notification");

    (void) memset(&eventCallbacks, 0, sizeof(eventCallbacks));
    eventCallbacks.Exception = &callbackExceptionEvent;


    error = jvmti->SetEventCallbacks(&eventCallbacks, (jint) sizeof(eventCallbacks));
    checkJVMTIError(jvmti, error, "Cannot set Event Callbacks.");


    error = jvmti->CreateRawMonitor("JVMTI Agent Data", &rawMonitorID);
    checkJVMTIError(jvmti, error, "Cannot create RAW Monitor");


    return JNI_OK;

}
