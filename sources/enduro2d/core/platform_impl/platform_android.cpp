/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform_android.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_ANDROID

#include <enduro2d/core/debug.hpp>
#include <enduro2d/core/vfs.hpp>
#include <enduro2d/utils/java.hpp>
#include <android/asset_manager_jni.h>

namespace
{
    using namespace e2d;

    //
    // asset_file_source
    //

    class asset_file_source final : public vfs::file_source {
    public:
        asset_file_source() noexcept = default;
        ~asset_file_source() noexcept final = default;
        bool valid() const noexcept final;
        bool exists(str_view path) const final;
        input_stream_uptr read(str_view path) const final;
        output_stream_uptr write(str_view path, bool append) const final;
        bool trace(str_view path, filesystem::trace_func func) const final;
    private:
        static AAssetManager* asset_manager() noexcept;
    };
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env;
    if ( vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK ) {
        return -1;
    }
    detail::java_vm::set(vm);

    try {
        java_class jc("enduro2d/engine/E2DNativeLib");
        jc.register_static_method("createPlatform", &e2d_native_lib::create_platform);
        jc.register_static_method("destroyPlatform", &e2d_native_lib::destroy_platform);
        jc.register_static_method("createWindow", &e2d_native_lib::create_window);
        jc.register_static_method("destroyWindow", &e2d_native_lib::destroy_window);
        jc.register_static_method("start", &e2d_native_lib::start);
        jc.register_static_method("stop", &e2d_native_lib::stop);
        jc.register_static_method("pause", &e2d_native_lib::pause);
        jc.register_static_method("resume", &e2d_native_lib::resume);
        jc.register_static_method("surfaceChanged", &e2d_native_lib::surface_changed);
        jc.register_static_method("surfaceDestroyed", &e2d_native_lib::surface_destroyed);
        jc.register_static_method("visibilityChanged", &e2d_native_lib::visibility_changed);
        jc.register_static_method("orientationChanged", &e2d_native_lib::orientation_changed);
        jc.register_static_method("onLowMemory", &e2d_native_lib::on_low_memory);
        jc.register_static_method("onTrimMemory", &e2d_native_lib::on_trim_memory);
        jc.register_static_method("tick", &e2d_native_lib::tick);
        jc.register_static_method("onKey", &e2d_native_lib::on_key);
        jc.register_static_method("onTouch", &e2d_native_lib::on_touch);
        jc.register_static_method("setApiVersion", &e2d_native_lib::set_api_version);
        jc.register_static_method("setDisplayInfo", &e2d_native_lib::set_display_info);
    } catch(...) {
        return -1;
    }

    return JNI_VERSION_1_6;
}

namespace e2d
{
    //
    // e2d_native_lib::platform_interface
    //

    class e2d_native_lib::platform_interface {
    public:
        platform_interface(java_obj&& context, java_obj&& asset_mngr, AAssetManager* jni_asset_mngr);
        [[nodiscard]] bool is_current_thread() const noexcept;
    public:
        java_obj context;
        java_obj asset_mngr;
        AAssetManager* jni_asset_mngr = nullptr;
    private:
        int android_version_ = 0;
        std::thread::id thread_id_ = std::this_thread::get_id();
    };

    e2d_native_lib::platform_interface::platform_interface(java_obj&& context, java_obj&& asset_mngr, AAssetManager* jni_asset_mngr)
    : context(std::move(context))
    , asset_mngr(std::move(asset_mngr))
    , jni_asset_mngr(jni_asset_mngr) {}

    bool e2d_native_lib::platform_interface::is_current_thread() const noexcept {
        return std::this_thread::get_id() == thread_id_;
    }
    
    /*void e2d_native_lib::platform_interface::set_android_version(int version) {
        E2D_ASSERT(is_current_thread());
        android_version_ = version;
    }*/

    //
    // e2d_native_lib::internal_state
    //
    
    e2d_native_lib::internal_state::internal_state() noexcept
    : platform_(nullptr, &destroy_platform_)
    , activity_(nullptr, &destroy_activity_)
    , renderer_(nullptr, &destroy_renderer_) {
    }

    e2d_native_lib::platform_interface& e2d_native_lib::internal_state::platform() const noexcept {
        E2D_ASSERT(platform_);
        return *platform_;
    }

    e2d_native_lib::activity_interface& e2d_native_lib::internal_state::activity() const noexcept {
        E2D_ASSERT(activity_);
        return *activity_;
    }

    e2d_native_lib::renderer_interface& e2d_native_lib::internal_state::renderer() const noexcept {
        E2D_ASSERT(renderer_);
        return *renderer_;
    }
    
    void e2d_native_lib::internal_state::destroy_platform_(platform_interface* ptr) {
        static_assert(sizeof(*ptr) > 0, "platform_interface must be defined");
        delete ptr;
    }

    //
    // e2d_native_lib
    //
    
    e2d_native_lib::internal_state& e2d_native_lib::state() noexcept {
        static e2d_native_lib::internal_state inst;
        return inst;
    }

    void JNICALL e2d_native_lib::create_platform(JNIEnv* env, jclass, jobject ctx, jobject asset_mngr) noexcept {
        auto* jni_asset_mngr = AAssetManager_fromJava(env, asset_mngr);
        state().platform_.reset(new platform_interface(java_obj(ctx), java_obj(asset_mngr), jni_asset_mngr));

        if ( !modules::is_initialized<debug>() ) {
            modules::initialize<debug>();
            the<debug>().register_sink<debug_console_sink>();
        }
    }

    void JNICALL e2d_native_lib::destroy_platform(JNIEnv*, jclass) noexcept {
        state().platform_.reset();
        modules::shutdown<debug>();
    }

    void JNICALL e2d_native_lib::on_low_memory(JNIEnv*, jclass) noexcept {
    }

    void JNICALL e2d_native_lib::on_trim_memory(JNIEnv*, jclass) noexcept {
    }

    void JNICALL e2d_native_lib::set_api_version(JNIEnv*, jclass, jint version) noexcept {
        try {
        } catch(const std::exception& e) {
            check_exceptions_(e);
        }
    }

    void e2d_native_lib::check_exceptions_(const std::exception& e) noexcept {
        __android_log_print(ANDROID_LOG_ERROR, "enduro2d", "exception: %s\n", e.what());
    }

    //
    // platform_internal_state_android
    //

    platform_internal_state_android::platform_internal_state_android(int argc, char *argv[])
    : internal_state(argc, argv) {}
    
    platform_internal_state_android::~platform_internal_state_android() noexcept {}
        
    void platform_internal_state_android::override_predef_paths(vfs& the_vfs) {
        the_vfs.register_scheme<asset_file_source>("assets");
        the_vfs.register_scheme_alias("resources", url{"assets", ""});
    }


    //
    // platform
    //

    platform::platform(int argc, char *argv[])
    : state_(new platform_internal_state_android(argc, argv)) {}
}

namespace
{
    //
    // android_input_stream
    //

    class android_input_stream final : public input_stream {
    public:
        android_input_stream(AAsset*);
        ~android_input_stream() noexcept;
        std::size_t read(void* dst, std::size_t size);
        std::size_t seek(std::ptrdiff_t offset, bool relative);
        std::size_t tell() const;
        std::size_t length() const noexcept;
    private:
        AAsset* asset_ = nullptr;
        std::size_t length_ = 0;
    };

    android_input_stream::android_input_stream(AAsset* asset)
    : asset_(asset)
    , length_(AAsset_getLength(asset_)) {
        if ( !asset ) {
            throw bad_stream_operation();
        }
    }

    android_input_stream::~android_input_stream() noexcept {
        if ( asset_ ) {
            AAsset_close(asset_);
        }
    }

    std::size_t android_input_stream::read(void* dst, std::size_t size) {
        return AAsset_read(asset_, dst, size);
    }
    
    std::size_t android_input_stream::seek(std::ptrdiff_t offset, bool relative) {
        return AAsset_seek(asset_, offset, relative ? SEEK_CUR : SEEK_SET);
    }

    std::size_t android_input_stream::tell() const {
        return length_ - AAsset_getRemainingLength(asset_);
    }

    std::size_t android_input_stream::length() const noexcept {
        return length_;
    }
    
    //
    // asset_file_source
    //
    
    AAssetManager* asset_file_source::asset_manager() noexcept {
        return e2d_native_lib::state().platform().jni_asset_mngr;
    }

    bool asset_file_source::valid() const noexcept {
        return asset_manager() != nullptr;
    }

    bool asset_file_source::exists(str_view path) const {
        AAsset* asset = AAssetManager_open(asset_manager(), path.data(), AASSET_MODE_UNKNOWN);
        if ( asset ) {
            AAsset_close(asset);
            return true;
        }
        return false;
    }

    input_stream_uptr asset_file_source::read(str_view path) const {
        AAsset* asset = AAssetManager_open(asset_manager(), path.data(), AASSET_MODE_UNKNOWN);
        return input_stream_uptr(new android_input_stream(asset));
    }

    output_stream_uptr asset_file_source::write(str_view path, bool append) const {
        throw std::runtime_error("can't write to assets");
        return nullptr;
    }

    bool asset_file_source::trace(str_view path, filesystem::trace_func func) const {
        /*if ( !func ) {
            return false;
        }
        str parent_path = "";

        const auto for_each_file_in_dir = [this] () {
            AAssetDir* dir = AAssetManager_openDir(asset_manager(), path.data());
            for (;;) {
                const char* asset_name = AAssetDir_getNextFileName(dir);
                if ( !asset_name ) {
                    break;
                }
                const str filename = path::combine(parent_path, asset_name);
                func(filename, false);
            }
        };*/
        return false;
    }
}

#endif
