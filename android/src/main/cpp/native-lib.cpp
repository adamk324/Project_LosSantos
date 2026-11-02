#include <jni.h>

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)vm;
    (void)reserved;
    return JNI_VERSION_1_6;
}

extern "C" void Java_com_project_los_santos_native_NativeBridge_helloWorld(JNIEnv* env, jobject obj) {
    (void)env;
    (void)obj;
}
