/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_ANDROID

#include <jni.h>

namespace e2d
{
    //
    // android_exception
    //

    class android_exception final : public std::exception {
    public:
        explicit android_exception(const char* msg) noexcept
        : msg_(msg) {}

        const char* what() const noexcept {
            return msg_;
        }

    private:
        const char* const msg_;
    };

    //
    // e2d_native_lib
    //

    class e2d_native_lib final {
    public:
        class platform_interface;
        class activity_interface; // UI thread
        class renderer_interface; // render thread

        class internal_state {
            friend class e2d_native_lib;
        public:
            [[nodiscard]] platform_interface& platform() const noexcept;
            [[nodiscard]] activity_interface& activity() const noexcept;
            [[nodiscard]] renderer_interface& renderer() const noexcept;
        private:
            internal_state() noexcept;
            static void destroy_platform_(platform_interface*);
            static void destroy_activity_(activity_interface*);
            static void destroy_renderer_(renderer_interface*);
        private:
            using platform_uptr = std::unique_ptr<platform_interface, void (*)(platform_interface*)>;
            using activity_uptr = std::unique_ptr<activity_interface, void (*)(activity_interface*)>;
            using renderer_uptr = std::unique_ptr<renderer_interface, void (*)(renderer_interface*)>;
            platform_uptr platform_;
            activity_uptr activity_;
            renderer_uptr renderer_;
        };
    private:
        static void check_exceptions_(JNIEnv* env, const std::exception& e) noexcept;
    public:
        [[nodiscard]] static internal_state& state() noexcept;

        // application
        static void JNICALL create_platform(JNIEnv*, jclass, jobject ctx, jobject asset_mngr) noexcept;
        static void JNICALL destroy_platform(JNIEnv*, jclass) noexcept;
        static void JNICALL on_low_memory(JNIEnv*, jclass) noexcept;
        static void JNICALL on_trim_memory(JNIEnv*, jclass) noexcept;
        static void JNICALL set_api_version(JNIEnv*, jclass, jint version) noexcept;
        static void JNICALL set_predef_path(JNIEnv*, jclass,
                                            jstring internal_appdata,
                                            jstring internal_cache,
                                            jstring external_appdata,
                                            jstring external_cache,
                                            jstring external_storage) noexcept;

        // window
        static void JNICALL create_window(JNIEnv*, jclass, jobject activity) noexcept;
        static void JNICALL destroy_window(JNIEnv*, jclass) noexcept;
        static void JNICALL start(JNIEnv*, jclass) noexcept;
        static void JNICALL stop(JNIEnv*, jclass) noexcept;
        static void JNICALL pause(JNIEnv*, jclass) noexcept;
        static void JNICALL resume(JNIEnv*, jclass) noexcept;
        static void JNICALL surface_changed(JNIEnv* env, jclass, jobject surface) noexcept;
        static void JNICALL surface_destroyed(JNIEnv*, jclass) noexcept;
        static void JNICALL visibility_changed(JNIEnv*, jclass) noexcept;
        static void JNICALL orientation_changed(JNIEnv*, jclass, jint value) noexcept;
        static void JNICALL tick(JNIEnv*, jclass) noexcept;
        static void JNICALL on_key(JNIEnv*, jclass, jint keycode, jint action) noexcept;
        static void JNICALL on_touch(JNIEnv*, jclass, jint action, jint num_pointers, jfloatArray touch_data_array) noexcept;
        static void JNICALL set_display_info(JNIEnv*, jclass, jint w, jint h, jint ppi) noexcept;
    };
}

#endif
