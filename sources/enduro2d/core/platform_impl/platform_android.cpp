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
#include <android/log.h>
#include <unistd.h>

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
        auto method_count = jc.static_method<jint()>("nativeMethodCount");
        const int expected = method_count();
        int count = 0;
        auto register_method = [&jc, &count](str_view name, auto* fn) {
            jc.register_static_method(name, fn);
            ++count;
        };
        register_method("createPlatform", &e2d_native_lib::create_platform);
        register_method("destroyPlatform", &e2d_native_lib::destroy_platform);
        register_method("createWindow", &e2d_native_lib::create_window);
        register_method("destroyWindow", &e2d_native_lib::destroy_window);
        register_method("start", &e2d_native_lib::start);
        register_method("stop", &e2d_native_lib::stop);
        register_method("pause", &e2d_native_lib::pause);
        register_method("resume", &e2d_native_lib::resume);
        register_method("surfaceChanged", &e2d_native_lib::surface_changed);
        register_method("surfaceDestroyed", &e2d_native_lib::surface_destroyed);
        register_method("visibilityChanged", &e2d_native_lib::visibility_changed);
        register_method("orientationChanged", &e2d_native_lib::orientation_changed);
        register_method("onLowMemory", &e2d_native_lib::on_low_memory);
        register_method("onTrimMemory", &e2d_native_lib::on_trim_memory);
        register_method("tick", &e2d_native_lib::tick);
        register_method("onKey", &e2d_native_lib::on_key);
        register_method("onTouch", &e2d_native_lib::on_touch);
        register_method("setDisplayInfo", &e2d_native_lib::set_display_info);
        register_method("setPredefPath", &e2d_native_lib::set_predef_path);

        if ( count != expected ) {
            __android_log_print(ANDROID_LOG_FATAL, "enduro2d", "%i native methods wasn't registered\n", expected - count);
            return -1;
        }
    } catch(const std::exception& e) {
        __android_log_print(ANDROID_LOG_FATAL, "enduro2d", "JNI_OnLoad failed: %s\n", e.what());
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
        [[nodiscard]] AAssetManager* asset_manager() const noexcept;
    public:
        std::mutex path_mutex;
        str internal_appdata_path;
        str internal_cache_path;
        str external_appdata_path;
        str external_cache_path;
        str external_storage_path;
    private:
        java_obj context_;
        java_obj asset_mngr_;
        AAssetManager* jni_asset_mngr_ = nullptr;
        std::thread::id thread_id_ = std::this_thread::get_id();
    };

    e2d_native_lib::platform_interface::platform_interface(java_obj&& context, java_obj&& asset_mngr, AAssetManager* jni_asset_mngr)
    : context_(std::move(context))
    , asset_mngr_(std::move(asset_mngr))
    , jni_asset_mngr_(jni_asset_mngr) {}

    bool e2d_native_lib::platform_interface::is_current_thread() const noexcept {
        return std::this_thread::get_id() == thread_id_;
    }
    
    AAssetManager* e2d_native_lib::platform_interface::asset_manager() const noexcept {
        return jni_asset_mngr_;
    }

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
    
    void JNICALL e2d_native_lib::set_predef_path(JNIEnv* env, jclass,
                                                 jstring internal_appdata,
                                                 jstring internal_cache,
                                                 jstring external_appdata,
                                                 jstring external_cache,
                                                 jstring external_storage) noexcept {
        try {
            auto& inst = state().platform();
            std::unique_lock<std::mutex> guard(inst.path_mutex);

            inst.internal_appdata_path = java_string(internal_appdata);
            inst.internal_cache_path = java_string(internal_cache);
            inst.external_appdata_path = java_string(external_appdata);
            inst.external_cache_path = java_string(external_cache);
            inst.external_storage_path = java_string(external_storage);

            // set default directory
            if ( chdir(inst.internal_appdata_path.data()) == -1 ) {
                the<debug>().error("can't set current directory to '%0'", inst.internal_appdata_path);
            }
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void e2d_native_lib::check_exceptions_(JNIEnv* env, const std::exception& e) noexcept {
        __android_log_print(ANDROID_LOG_ERROR, "enduro2d", "exception: %s\n", e.what());
        env->ExceptionClear();
    }
    
    //
    // platform_internal_state_android
    //

    class platform_internal_state_android final : public platform::internal_state {
    public:
        platform_internal_state_android(int argc, char *argv[]);
        ~platform_internal_state_android() noexcept = default;
        
        void register_scheme_aliases(vfs&) override;
    };

    platform_internal_state_android::platform_internal_state_android(int argc, char *argv[])
    : internal_state(argc, argv) {}
        
    void platform_internal_state_android::register_scheme_aliases(vfs& the_vfs) {
        auto& inst = e2d_native_lib::state().platform();
        std::unique_lock<std::mutex> guard(inst.path_mutex);

        the_vfs.register_scheme<asset_file_source>("assets");
        the_vfs.register_scheme_alias("resources", url{"assets", ""});

        the_vfs.register_scheme<filesystem_file_source>("file");
        the_vfs.register_scheme_alias("home", url("file", inst.external_storage_path));
        the_vfs.register_scheme_alias("appdata", url("file", inst.external_appdata_path));
        the_vfs.register_scheme_alias("desktop", url("file", inst.external_storage_path));
        the_vfs.register_scheme_alias("working", url("file", inst.internal_appdata_path));
        the_vfs.register_scheme_alias("documents", url("file", inst.external_storage_path));
        the_vfs.register_scheme_alias("executable", url("file", inst.internal_appdata_path));
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
        ~android_input_stream() noexcept override;
        std::size_t read(void* dst, std::size_t size) override;
        std::size_t seek(std::ptrdiff_t offset, bool relative) override;
        std::size_t tell() const  override;
        std::size_t length() const noexcept override;
    private:
        AAsset* asset_ = nullptr;
        std::size_t length_ = 0;
    };

    android_input_stream::android_input_stream(AAsset* asset)
    : asset_(asset) {
        if ( !asset ) {
            throw bad_stream_operation();
        }
        length_ = math::numeric_cast<size_t>(AAsset_getLength(asset_));
    }

    android_input_stream::~android_input_stream() noexcept {
        if ( asset_ ) {
            AAsset_close(asset_);
        }
    }

    std::size_t android_input_stream::read(void* dst, std::size_t size) {
        off_t rread = AAsset_read(asset_, dst, size);
        return rread >= 0
            ? math::numeric_cast<size_t>(rread)
            : throw bad_stream_operation();
    }
    
    std::size_t android_input_stream::seek(std::ptrdiff_t offset, bool relative) {
        off_t rseek = AAsset_seek(asset_, offset, relative ? SEEK_CUR : SEEK_SET);
        return rseek >= 0
            ? math::numeric_cast<size_t>(rseek)
            : throw bad_stream_operation();
    }

    std::size_t android_input_stream::tell() const {
        return length_ - math::numeric_cast<size_t>(AAsset_getRemainingLength(asset_));
    }

    std::size_t android_input_stream::length() const noexcept {
        return length_;
    }
    
    //
    // asset_file_source
    //
    
    AAssetManager* asset_file_source::asset_manager() noexcept {
        return e2d_native_lib::state().platform().asset_manager();
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
        try {
            return input_stream_uptr(new android_input_stream(asset));
        } catch(...) {
            return nullptr;
        }
    }

    output_stream_uptr asset_file_source::write(str_view path, bool append) const {
        E2D_UNUSED(path, append);
        throw bad_stream_operation();
        return nullptr;
    }

    bool asset_file_source::trace(str_view path, filesystem::trace_func func) const {
        if ( !func ) {
            return false;
        }

        AAssetDir* dir = AAssetManager_openDir(asset_manager(), path.data());
        for (;;) {
            const char* asset_name = AAssetDir_getNextFileName(dir);
            if ( !asset_name ) {
                break;
            }
            const str filename = path::combine(path, asset_name);
            if ( !func(filename, false) ) {
                return false;
            }
        }
        return true;
    }
}

#endif
