/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_ANDROID

#include <enduro2d/utils/java.hpp>

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
}

namespace e2d
{
    platform_internal_state_impl_uptr platform_internal_state_impl::create() {
        return std::make_unique<platform_internal_state_impl_android>();
    }
}

// TODO: remove
int e2d_main(int argc, char *argv[]) {
    return 0;
}
#endif
