/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#ifdef __INTELLISENSE__
#  define E2D_PLATFORM_MODE E2D_PLATFORM_MODE_ANDROID
#endif

#include "platform_android.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_ANDROID

#include <enduro2d/utils/java.hpp>
#include <android/asset_manager_jni.h>

namespace
{
    using namespace e2d;

    //
    // android_interface
    //

    class android_interface {
    public:
        void release() noexcept;
        [[nodiscard]] static android_interface& instance() noexcept;
    public:
        java_obj context;
        java_obj asset_mngr;
    };

    android_interface& android_interface::instance() noexcept {
        static android_interface inst;
        return inst;
    }
        
    void android_interface::release() noexcept {
        asset_mngr = {};
        context = {};
    }


    //
    // jni
    //

    extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
        JNIEnv* env;
        if ( vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK ) {
            return -1;
        }
        detail::java_vm::set(vm);
        return JNI_VERSION_1_6;
    }
    
    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_initialize(JNIEnv*, jclass, jobject ctx, jobject asset_mngr) noexcept {
        auto& inst = android_interface::instance();
        inst.context = java_obj(ctx);
        inst.asset_mngr = java_obj(asset_mngr);
    }
}

namespace e2d
{
    //
    // platform_internal_state_android
    //

    platform_internal_state_android::platform_internal_state_android(int argc, char *argv[])
    : internal_state(argc, argv) {
        java_env je;
        asset_manager_ = AAssetManager_fromJava(je.env(), android_interface::instance().asset_mngr.get());
    }
    
    platform_internal_state_android::~platform_internal_state_android() noexcept {
        android_interface::instance().release();
    }

    AAssetManager* platform_internal_state_android::asset_manager() const noexcept {
        return asset_manager_;
    }

    //
    // platform
    //

    platform::platform(int argc, char *argv[])
    : state_(new platform_internal_state_android(argc, argv)) {}
}

#endif
