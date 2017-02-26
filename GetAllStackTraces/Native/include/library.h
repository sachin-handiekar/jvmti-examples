#ifndef TINYOPS_LIBRARY_H
#define TINYOPS_LIBRARY_H

void fatalError(const char * format, ...);

static bool checkJVMTIError(jvmtiEnv *jvmti, jvmtiError errNum, const char *str);

#endif