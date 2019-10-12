#include <jni.h>
#include <string>
#include <SDL.h>

extern "C" JNIEXPORT jstring JNICALL
Java_org_libsdl_app_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
