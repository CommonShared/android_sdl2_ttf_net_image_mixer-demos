/*
    SDL_android_main.c, placed in the public domain by Sam Lantinga  3/13/14
*/
#include "SDL_internal.h"

#ifdef __ANDROID__

/* Include the SDL main definition header */
#include "SDL_main.h"

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
#include <jni.h>

/* Called before SDL_main() to initialize JNI bindings in SDL library */
//extern void SDL_Android_Init(JNIEnv* env, jclass cls);
//extern void nativeSetupJNI(JNIEnv* env, jclass cls);
/* This prototype is needed to prevent a warning about the missing prototype for global function below */
//JNIEXPORT int JNICALL Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject array);

/* Start up the SDL app */
extern "C" JNIEXPORT int JNICALL Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jstring jinput)
{
    int i;
    int argc;
    int status;
    int len;
    char* argv[3];

    /* This interface could expand with ABI negotiation, callbacks, etc. */
    //SDL_Android_Init(env, cls);
    //nativeSetupJNI(env, cls);

    SDL_SetMainReady();

    /* Prepare the arguments. */

    argv[0] = SDL_strdup("app_process");
    const char* utf = NULL;
	if(jinput){
		utf = env->GetStringUTFChars(jinput, 0);
		if(utf){
			argv[1] = SDL_strdup(utf);
			env->ReleaseStringUTFChars(jinput, utf);
		}
	}
	argv[2] = NULL;

    /* Run the application. */

    status = SDL_main(argc, argv);

    /* Release the arguments. */

    for (i = 0; i < argc; ++i) {
        SDL_free(argv[i]);
    }
    SDL_stack_free(argv);
    /* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
    /* exit(status); */

    return status;
}

#endif /* __ANDROID__ */

/* vi: set ts=4 sw=4 expandtab: */
