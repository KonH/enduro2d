/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "window.hpp"

#if 1 //defined(E2D_WINDOW_MODE) && E2D_WINDOW_MODE == E2D_WINDOW_MODE_NATIVE_ANDROID

#include <enduro2d/utils/java.hpp>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <EGL/egl.h>

namespace
{
    using namespace e2d;

    //
    // message_queue
    //

    template < typename T >
    class message_queue {
    public:
        message_queue();
        template < typename FN >
        void process(FN&&);
        void push(const T&);
    private:
        std::mutex lock_;
        std::vector<T> queue_;  // TODO: optimize
    };

    //
    // android_activity (UI thread)
    //

    class android_activity {
    public:
        enum class msg_type : u32 {
            set_orientation,
        };
        struct orientation {
            int value;
        };
        struct message {
            msg_type type;
            union {
                orientation orient;
            };
        };
        using msg_queue_t = message_queue<message>;
    public:
        android_activity() noexcept;
        void push_msg(const message& msg);
        void process_messages();
        void set_activity(jobject);
        [[nodiscard]] bool is_current_thread() const noexcept;
    public:
        java_obj main_activity_;
        msg_queue_t messages_;
        std::thread::id thread_id_ = std::this_thread::get_id();
    };

    //
    // android_surface
    //

    class android_surface {
    public:
        explicit android_surface(debug&) noexcept;
        ~android_surface() noexcept;
        void create_context(v4i rgba_size, i32 depth, i32 stencil, i32 samples);
        void destroy_context() noexcept;
        void create_surface(jobject window);
        void destroy_surface() noexcept;
        void bind_context() noexcept;
        void swap_buffers() noexcept;
        [[nodiscard]] bool has_context() const noexcept;
        [[nodiscard]] bool has_surface() const noexcept;
        [[nodiscard]] v2u size() const noexcept;
    private:
        EGLConfig config_ = nullptr;
        EGLDisplay display_ = EGL_NO_DISPLAY;
        EGLSurface surface_ = EGL_NO_SURFACE;
        EGLContext context_ = EGL_NO_CONTEXT;
        EGLNativeWindowType window_ = nullptr;
        int egl_version_ = 0;
        debug& debug_;
    };

    //
    // android_window (render thread)
    //

    class android_window {
    public:
        enum class msg_type : u32 {
            app_create,
            app_destroy,
            app_start,
            app_stop,
            app_pause,
            app_resume,
            surface_changed,
            orientation_changed,
            key,
            touch,
        };

        struct surface_data {
            jobject surface;
            int width;
            int height;
        };

        struct orientation {
            int value;
        };

        struct key {
            int code;
            bool down;
        };

        struct touch {
            int action;
        };

        struct message {
            msg_type type;
            union {
                surface_data surface_data;
                orientation orientation;
                key key;
                touch touch;
            };
        };
        using event_listener_uptr = window::event_listener_uptr;
        using listeners_t = vector<event_listener_uptr>;
        using msg_queue_t = message_queue<message>;
    public:
        android_window() noexcept;
        void quit();
        void push_msg(const message& msg);
        [[nodiscard]] bool is_current_thread() const noexcept;
    
        template < typename F, typename... Args >
        void for_all_listeners(const F& f, const Args&... args) noexcept;
    private:
        void render_loop_() noexcept;
        void process_messages_();
        void on_destroy_();
        void on_surface_changed_(const surface_data&);
        void on_key_(const key&);
        void on_touch_(const touch&);
        void on_orientation_changed_(const orientation&);
    public:
        std::recursive_mutex rmutex;
        listeners_t listeners;
        v2u real_size;
        v2u virtual_size;
        v2u framebuffer_size;
        bool fullscreen = false;
        bool enabled = true;
        bool visible = true;
        bool focused = true;
    private:
        android_surface surface_;
        msg_queue_t messages_;
        std::thread thread_;
        std::atomic<bool> exit_loop_ = false;
    };

    //
    // java_interface
    //

    class java_interface {
    public:
        java_interface() noexcept = default;
        [[nodiscard]] static java_interface& instance() noexcept;
    public:
        android_activity activity;
        android_window window;
    };
}

namespace
{
    //
    // java_interface
    //

    java_interface& java_interface::instance() noexcept {
        static java_interface inst;
        return inst;
    }

    //
    // message_queue
    //
    
    template < typename T >
    message_queue<T>::message_queue() {
        std::unique_lock<std::mutex> guard(lock_);
        queue_.reserve(64);
    }
    
    template < typename T >
    template < typename FN >
    void message_queue<T>::process(FN&& fn) {
        for (;;) {
            T temp;
            {
                std::unique_lock<std::mutex> guard(lock_);
                if ( queue_.empty() )
                    return;
                temp = std::move(queue_.front());
            }
            fn(temp);
        }
    }
    
    template < typename T >
    void message_queue<T>::push(const T& msg) {
        std::unique_lock<std::mutex> guard(lock_);
        queue_.push_back(std::move(msg));
    }
    
    //
    // android_activity
    //
    
    android_activity::android_activity() noexcept {
    }

    void android_activity::set_activity(jobject obj) {
        E2D_ASSERT(is_current_thread());
        main_activity_ = java_obj(obj);
    }
    
    void android_activity::push_msg(const message& msg) {
        messages_.push(msg);
    }

    void android_activity::process_messages() {
        E2D_ASSERT(is_current_thread());
        messages_.process([this] (auto& msg) {
            switch( msg.type ) {
                case msg_type::set_orientation :
                    break;
            }
        });
    }
    
    bool android_activity::is_current_thread() const noexcept {
        return std::this_thread::get_id() == thread_id_;
    }

    //
    // android_surface
    //

    #if defined(E2D_BUILD_MODE) && E2D_BUILD_MODE == E2D_BUILD_MODE_DEBUG
    #   define EGL_CHECK_CODE(dbg, code)\
            code;\
            for ( EGLint err = eglGetError(); err != EGL_SUCCESS; err = eglGetError() ) {\
                E2D_ASSERT_MSG(false, #code);\
                (dbg).log(err == EGL_CONTEXT_LOST\
                    ? debug::level::fatal\
                    : debug::level::error,\
                    "ANDROID: EGL_CHECK(%0):\n"\
                    "--> File: %1\n"\
                    "--> Line: %2\n"\
                    "--> Code: %3",\
                    #code, __FILE__, __LINE__, egl_error_to_cstr(err));\
                if ( err == EGL_CONTEXT_LOST ) std::terminate();\
            }
    #else
    #   define EGL_CHECK_CODE(dbg, code) E2D_UNUSED(dbg); code;
    #endif
    
    const char* egl_error_to_cstr(EGLint err) noexcept {
        #define DEFINE_CASE(x) case x: return #x
        switch ( err ) {
            DEFINE_CASE(EGL_NOT_INITIALIZED);
            DEFINE_CASE(EGL_BAD_ACCESS);
            DEFINE_CASE(EGL_BAD_ALLOC);
            DEFINE_CASE(EGL_BAD_ATTRIBUTE);
            DEFINE_CASE(EGL_BAD_CONFIG);
            DEFINE_CASE(EGL_BAD_CONTEXT);
            DEFINE_CASE(EGL_BAD_CURRENT_SURFACE);
            DEFINE_CASE(EGL_BAD_DISPLAY);
            DEFINE_CASE(EGL_BAD_MATCH);
            DEFINE_CASE(EGL_BAD_NATIVE_PIXMAP);
            DEFINE_CASE(EGL_BAD_NATIVE_WINDOW);
            DEFINE_CASE(EGL_BAD_PARAMETER);
            DEFINE_CASE(EGL_BAD_SURFACE);
            DEFINE_CASE(EGL_CONTEXT_LOST);
            default :
                return "EGL_UNKNOWN";
        }
        #undef DEFINE_CASE
    }
    
    android_surface::android_surface(debug& dbg) noexcept
    : debug_(dbg) {
    }

    android_surface::~android_surface() noexcept {
        E2D_ASSERT(!context_);
        E2D_ASSERT(!window_);
    }

    void android_surface::create_context(v4i rgba_size, i32 depth, i32 stencil, i32 samples) {
        EGL_CHECK_CODE(debug_, display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY));
        if ( display_ == EGL_NO_DISPLAY )
            throw std::runtime_error("failed to get EGL display");

        EGLint maj_ver, min_ver;
        EGLBoolean ok;
        EGL_CHECK_CODE(debug_, ok = eglInitialize(display_, &maj_ver, &min_ver));
        if ( ok != EGL_TRUE )
            throw std::runtime_error("failed to initialize EGL");

        egl_version_ = maj_ver * 100 + min_ver * 10;
        EGL_CHECK_CODE(debug_, eglBindAPI(EGL_OPENGL_ES_API));

        const EGLint required_config[] = {
            EGL_SURFACE_TYPE,     EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE,  EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE,         rgba_size[0],
            EGL_GREEN_SIZE,       rgba_size[1],
            EGL_BLUE_SIZE,        rgba_size[2],
            EGL_ALPHA_SIZE,       rgba_size[2],
            EGL_DEPTH_SIZE,       depth,
            EGL_STENCIL_SIZE,     stencil,
            EGL_SAMPLES,          samples,
            EGL_NONE
        };
        EGLConfig configs[1024];
        EGLint num_configs = 0;
        EGL_CHECK_CODE(debug_, ok = eglGetConfigs(display_, configs, std::size(configs), &num_configs));
        if ( ok != EGL_TRUE )
            throw std::runtime_error("failed to get EGL display configs");

        EGL_CHECK_CODE(debug_, ok = eglChooseConfig(display_, required_config, configs, std::size(configs), &num_configs));
        if ( ok != EGL_TRUE || num_configs == 0 )
            throw std::runtime_error("failed to choose EGL display config");

        const auto get_attrib = [this] (EGLConfig cfg, EGLint attrib) {
            EGLint result = 0;
            EGLBoolean ok;
            EGL_CHECK_CODE(debug_, ok = eglGetConfigAttrib(display_, cfg, attrib, &result));
            return ok == EGL_TRUE ? result : 0;
        };
        config_ = nullptr;

        for ( EGLint i = 0; i < num_configs; ++i ) {
            EGLint depth = get_attrib(configs[i], EGL_DEPTH_SIZE);
            EGLint stencil = get_attrib(configs[i], EGL_STENCIL_SIZE);
            EGLint r = get_attrib(configs[i], EGL_RED_SIZE);
            EGLint g = get_attrib(configs[i], EGL_GREEN_SIZE);
            EGLint b = get_attrib(configs[i], EGL_BLUE_SIZE);
            EGLint a = get_attrib(configs[i], EGL_ALPHA_SIZE);
            EGLint samples = get_attrib(configs[i], EGL_SAMPLES);
        }
        if ( !config_ ) {
            config_ = configs[0];
        }
        const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        EGL_CHECK_CODE(debug_, context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attribs));
        if ( context_ == EGL_NO_CONTEXT )
            throw std::runtime_error("failed to create EGL context");
    }

    void android_surface::destroy_context() noexcept {
        destroy_surface();
        if ( context_ != EGL_NO_CONTEXT ) {
            EGL_CHECK_CODE(debug_, eglDestroyContext(display_, context_));
            context_ = EGL_NO_CONTEXT;
        }
        if ( display_ != EGL_NO_DISPLAY ) {
            EGL_CHECK_CODE(debug_, eglTerminate(display_));
            display_ = EGL_NO_DISPLAY;
        }
    }

    void android_surface::create_surface(jobject window) {
        destroy_surface();

        java_env je;
        window_ = ANativeWindow_fromSurface(je.env(), window);
        
        EGLint format = 0;
        EGL_CHECK_CODE(debug_, eglGetConfigAttrib(display_, config_, EGL_NATIVE_VISUAL_ID, &format));

        if ( !ANativeWindow_setBuffersGeometry(window_, 0, 0, format) )
            throw std::runtime_error("failed to set pixel format to native window");

        const EGLint surface_attribs[] = {
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE
        };
        EGL_CHECK_CODE(debug_,  surface_ = eglCreateWindowSurface(display_, config_, window_, surface_attribs));
        if ( surface_ == EGL_NO_SURFACE )
            throw std::runtime_error("failed to create window surface");
            
        EGLBoolean ok;
        EGL_CHECK_CODE(debug_, ok = eglMakeCurrent(display_, surface_, surface_, context_));
        if ( ok != EGL_TRUE )
            throw std::runtime_error("failed to make EGL context current");
    }

    void android_surface::destroy_surface() noexcept {
        if ( surface_ != EGL_NO_SURFACE ) {
            EGL_CHECK_CODE(debug_, eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
            EGL_CHECK_CODE(debug_, eglDestroySurface(display_, surface_));
            surface_ = EGL_NO_SURFACE;
        }
        if ( window_ ) {
            ANativeWindow_release(window_);
            window_ = nullptr;
        }
    }

    void android_surface::bind_context() noexcept {
        EGL_CHECK_CODE(debug_, eglMakeCurrent(display_, surface_, surface_, context_));
    }

    void android_surface::swap_buffers() noexcept {
        E2D_ASSERT(display_ != EGL_NO_DISPLAY);
        E2D_ASSERT(surface_ != EGL_NO_SURFACE);
        E2D_ASSERT(context_ != EGL_NO_CONTEXT && context_ == eglGetCurrentContext());
        EGL_CHECK_CODE(debug_, eglSwapBuffers(display_, surface_));
    }
    
    bool android_surface::has_context() const noexcept {
        return context_ != EGL_NO_CONTEXT;
    }
    
    bool android_surface::has_surface() const noexcept {
        return surface_ != EGL_NO_SURFACE;
    }

    v2u android_surface::size() const noexcept {
        E2D_ASSERT(display_ != EGL_NO_DISPLAY);
        E2D_ASSERT(surface_ != EGL_NO_SURFACE);
        v2u result;
        EGL_CHECK_CODE(debug_, eglQuerySurface(display_, surface_, EGL_WIDTH, reinterpret_cast<EGLint*>(&result.x)));
        EGL_CHECK_CODE(debug_, eglQuerySurface(display_, surface_, EGL_HEIGHT, reinterpret_cast<EGLint*>(&result.y)));
        return result;
    }
    
    //
    // android_window
    //
    
    android_window::android_window() noexcept
    : surface_(the<debug>()) {
        thread_ = std::thread([this]() { render_loop_(); });
    }
    
    void android_window::quit() {
        thread_.join();
    }

    void android_window::render_loop_() noexcept {
        E2D_ASSERT(is_current_thread());
        bool main_was_called = false;

        for (; !exit_loop_.load(std::memory_order_relaxed); ) {
            try {
                process_messages_();
            } catch(...) {
                // TODO
            }
            if ( !main_was_called && surface_.has_surface() ) {
                main_was_called = true;
                e2d_main(0, nullptr);
            }
        }
    }

    void android_window::push_msg(const message& msg) {
        messages_.push(msg);
    }

    void android_window::process_messages_() {
        messages_.process([this] (auto& msg) {
            switch( msg.type ) {
                case msg_type::app_create :
                    break;
                case msg_type::app_destroy :
                    on_destroy_();
                    break;
                case msg_type::app_start :
                    break;
                case msg_type::app_stop :
                    break;
                case msg_type::app_pause :
                    break;
                case msg_type::app_resume :
                    break;
                case msg_type::surface_changed :
                    on_surface_changed_(msg.surface_data);
                    break;
                case msg_type::orientation_changed :
                    on_orientation_changed_(msg.orientation);
                    break;
                case msg_type::key :
                    on_key_(msg.key);
                    break;
                case msg_type::touch :
                    on_touch_(msg.touch);
                    break;
            }
        });
    }
    
    void android_window::on_destroy_() {
        // TODO
        // ...
        surface_.destroy_context();
        exit_loop_.store(true, std::memory_order_relaxed);
    }

    void android_window::on_surface_changed_(const surface_data& data) {
        if ( data.surface ) {
            if ( !surface_.has_context() ) {
                surface_.create_context(v4i(8,8,8,0), 16, 0, 0);
                __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "egl context initialized\n");
            }
            surface_.create_surface(data.surface);
        } else {
            surface_.destroy_surface();
        }
    }
     
    void android_window::on_key_(const key& data) {
        // TODO
    }

    void android_window::on_touch_(const touch& data) {
        // TODO
    }
    
    void android_window::on_orientation_changed_(const orientation& data) {
        // TODO
    }

    template < typename F, typename... Args >
    void android_window::for_all_listeners(const F& f, const Args&... args) noexcept {
        std::lock_guard<std::recursive_mutex> guard(rmutex);
        for ( const event_listener_uptr& listener : listeners ) {
            if ( listener ) {
                stdex::invoke(f, listener.get(), args...);
            }
        }
    }

    bool android_window::is_current_thread() const noexcept {
        return std::this_thread::get_id() == thread_.get_id();
    }
}

namespace e2d
{
    //
    // window::state
    //

    class window::state final : private e2d::noncopyable {
    public:
        state() noexcept = default;
        ~state() noexcept = default;

        std::recursive_mutex& rmutex() noexcept {
            return java_interface::instance().window.rmutex;
        }

        auto& listeners() noexcept {
            return java_interface::instance().window.listeners;
        }
    };

    window::window(const v2u& size, str_view title, bool vsync, bool fullscreen)
    : state_(new state()) {
        E2D_UNUSED(size, title, vsync, fullscreen);
    }

    window::~window() noexcept = default;

    void window::hide() noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::show() noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::restore() noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::minimize() noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::enabled() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::visible() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::focused() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::minimized() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::fullscreen() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::toggle_fullscreen(bool yesno) noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::hide_cursor() noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::show_cursor() noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::is_cursor_hidden() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    v2u window::real_size() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    v2u window::virtual_size() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    v2u window::framebuffer_size() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    const str& window::title() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::set_title(str_view title) {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    bool window::should_close() const noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::set_should_close(bool yesno) noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        // TODO
    }

    void window::bind_context() noexcept {
    }

    void window::swap_buffers() noexcept {
    }

    bool window::poll_events() noexcept {
        return false;
    }

    window::event_listener& window::register_event_listener(event_listener_uptr listener) {
        E2D_ASSERT(listener);
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        state_->listeners().push_back(std::move(listener));
        return *state_->listeners().back();
    }

    void window::unregister_event_listener(const event_listener& listener) noexcept {
        std::lock_guard<std::recursive_mutex> guard(state_->rmutex());
        for ( auto iter = state_->listeners().begin(); iter != state_->listeners().end(); ) {
            if ( iter->get() == &listener ) {
                iter = state_->listeners().erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

namespace
{
    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_create (JNIEnv* env, jobject, jobject activity) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_create\n");

            if ( !modules::is_initialized<debug>() ) {
                modules::initialize<debug>();
                the<debug>().register_sink<debug_native_log_sink>();
            }
            auto& inst = java_interface::instance();
            inst.activity.set_activity(activity);

            android_window::message msg = {};
            msg.type = android_window::msg_type::app_create;
            inst.window.push_msg(msg);
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_destroy (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_destroy\n");

            android_window::message msg = {};
            msg.type = android_window::msg_type::app_destroy;

            auto& inst = java_interface::instance();
            inst.window.push_msg(msg);
            inst.window.quit();
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_start (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_start\n");

            android_window::message msg = {};
            msg.type = android_window::msg_type::app_start;
            java_interface::instance().window.push_msg(msg);
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_stop (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_stop\n");

            android_window::message msg = {};
            msg.type = android_window::msg_type::app_stop;
            java_interface::instance().window.push_msg(msg);
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_pause (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_pause\n");

            android_window::message msg = {};
            msg.type = android_window::msg_type::app_pause;
            java_interface::instance().window.push_msg(msg);
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_resume (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_resume\n");

            android_window::message msg = {};
            msg.type = android_window::msg_type::app_resume;
            java_interface::instance().window.push_msg(msg);
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_surfaceChanged (JNIEnv* env, jobject obj, jobject surface, jint w, jint h) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_surfaceChanged\n");

            android_window::message msg = {};
            msg.type = android_window::msg_type::surface_changed;
            msg.surface_data.surface = surface;
            msg.surface_data.width = w;
            msg.surface_data.height = h;
            java_interface::instance().window.push_msg(msg);
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_surfaceDestroyed (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_surfaceDestroyed\n");

            android_window::message msg = {};
            msg.type = android_window::msg_type::surface_changed;
            java_interface::instance().window.push_msg(msg);
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_visibilityChanged (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_visibilityChanged\n");
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_orientationChanged (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_orientationChanged\n");
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_onLowMemory (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_onLowMemory\n");
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_onTrimMemory (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_onTrimMemory\n");
        } catch(...) {
            // TODO ?
        }
    }

    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_tick (JNIEnv* env, jobject obj) noexcept {
        try {
            __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "E2DNativeLib_tick\n");
            java_interface::instance().activity.process_messages();
        } catch(...) {
            // TODO ?
        }
    }
}
#endif
