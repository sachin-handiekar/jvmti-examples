#include <jvmti.h>
#include <stdio.h>

static void listCapabilities(jvmtiEnv *jvmti) {
    jvmtiCapabilities caps;
    jvmtiError err = (*jvmti)->GetPotentialCapabilities(jvmti, &caps);
    if (err != JVMTI_ERROR_NONE) {
        printf("Failed to get capabilities: %d\n", err);
        return;
    }
    printf("--- Potential JVMTI Capabilities ---\n");
    printf("can_tag_objects: %d\n", caps.can_tag_objects);
    printf("can_generate_field_modification_events: %d\n", caps.can_generate_field_modification_events);
    printf("can_generate_method_entry_events: %d\n", caps.can_generate_method_entry_events);
    printf("can_generate_exception_events: %d\n", caps.can_generate_exception_events);
    printf("can_generate_garbage_collection_events: %d\n", caps.can_generate_garbage_collection_events);
    // Print more if needed
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("[Learning JVMTI (Author : Sachin Handiekar)]\n");
    printf("[Chapter - 01 - capability-checker]\n");
    printf("** Agent loaded **\n");

    jvmtiEnv *jvmti = NULL;
    jint res = (*vm)->GetEnv(vm, (void **) &jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("Unable to access JVMTI!\n");
        return JNI_ERR;
    }
    listCapabilities(jvmti);
    return JNI_OK;
}
