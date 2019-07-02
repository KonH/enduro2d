/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_ANDROID

#include <android/native_window.h>
#include <android/native_window_jni.h>

namespace
{
    using namespace e2d;

    //
    // platform_internal_state_impl_android
    //

    class platform_internal_state_impl_android final : public platform_internal_state_impl {
    public:
        platform_internal_state_impl_android() = default;
        ~platform_internal_state_impl_android() noexcept final = default;
    };

    extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
        JNIEnv* env;
        if ( vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK ) {
            return -1;
        }
        detail::java_vm::set(vm);
        return JNI_VERSION_1_6;
    }

    //
    // java_interface
    //

    class java_interface {
    public:
        java_interface() noexcept = default;
        [[nodiscard]] static java_interface& instance() noexcept;
    public:
        java_obj main_activity_;
    };
    
    java_interface& java_interface::instance() noexcept {
        static java_interface inst;
        return inst;
    }

    //
    // android_window
    //

    class android_window {
    public:
        android_window() {}
        android_window(JNIEnv* env, jobject surface);
        android_window(android_window&&);
        ~android_window();
    private:
        ANativeWindow* window_ {nullptr};
    };
    
    android_window::android_window(JNIEnv* env, jobject surface) {
        if ( surface ) {
            window_ = ANativeWindow_fromSurface(env, surface);
        }
    }

    android_window::android_window(android_window&& other)
    : window_(other.window_) {
        other.window_ = nullptr;
    }

    android_window::~android_window() {
        if ( window_ ) {
            ANativeWindow_release(window_);
        }
    }
}

namespace
{
    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_create (JNIEnv* env, jobject, jobject activity) {
        auto& inst = java_interface::instance();
        inst.main_activity_ = java_obj(activity);
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_destroy (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_start (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_stop (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_pause (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_resume (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_surfaceChanged (JNIEnv* env, jobject obj, jobject surface, jint w, jint h) {
        android_window wnd(env, surface);
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_surfaceDestroyed (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_visibilityChanged (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_orientationChanged (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_onLowMemory (JNIEnv* env, jobject obj) {
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_onTrimMemory (JNIEnv* env, jobject obj) {
    }
}

namespace e2d
{
    platform_internal_state_impl_uptr platform_internal_state_impl::create() {
        return std::make_unique<platform_internal_state_impl_android>();
    }
}

int e2d_main(int argc, char *argv[]) {
    return 0;
}
#endif
