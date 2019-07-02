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

namespace
{
    using namespace e2d;
    
    //
    // android_surface
    //

    class android_surface {
    public:
        android_surface() {}
        android_surface(JNIEnv* env, jobject surface);
        android_surface(android_surface&&);
        android_surface& operator = (android_surface&&);
        ~android_surface();
    private:
        ANativeWindow* window_ {nullptr};
    };

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
        template < typename T >
        void push_msg(const T& msg);
        void process_messages();
        void set_activity(jobject);
    public:
        java_obj main_activity_;
        msg_queue_t messages_;
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
            surface_destroyed,
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
        template < typename T >
        void push_msg(const T& msg);
        void process_messages();
        void on_surface_changed(const surface_data&);
        void on_key(const key&);
        void on_touch(const touch&);
        void on_orientation_changed(const orientation&);
    
        template < typename F, typename... Args >
        void for_all_listeners(const F& f, const Args&... args) noexcept;
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
    };

    //
    // java_interface
    //

    class java_interface {
    public:
        java_interface() = default;
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
    // android_surface
    //

    android_surface::android_surface(JNIEnv* env, jobject surface) {
        if ( surface ) {
            window_ = ANativeWindow_fromSurface(env, surface);
        }
    }

    android_surface::android_surface(android_surface&& other)
    : window_(other.window_) {
        other.window_ = nullptr;
    }
    
    android_surface& android_surface::operator = (android_surface&& rhs) {
        if ( window_ ) {
            ANativeWindow_release(window_);
        }
        window_ = rhs.window_;
        rhs.window_ = nullptr;
    }

    android_surface::~android_surface() {
        if ( window_ ) {
            ANativeWindow_release(window_);
        }
    }
    
    //
    // android_activity
    //
    
    void android_activity::set_activity(jobject obj) {
        main_activity_ = java_obj(obj);
    }

    template < typename T >
    void android_activity::push_msg(const T& msg) {
        messages_.push(msg);
    }

    void android_activity::process_messages() {
        messages_.process([this] (auto& msg) {
            switch( msg.type ) {
            case msg_type::set_orientation :
                break;
            }
        });
    }
    
    //
    // android_window
    //
        
    template < typename T >
    void android_window::push_msg(const T& msg) {
        messages_.push(msg);
    }

    void android_window::process_messages() {
        messages_.process([this] (auto& msg) {
            switch( msg.type ) {
            case msg_type::app_create :
                break;
            case msg_type::app_destroy :
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
                on_surface_changed(msg.surface_data);
                break;
            case msg_type::surface_destroyed :
                break;
            case msg_type::orientation_changed :
                on_orientation_changed(msg.orientation);
                break;
            case msg_type::key :
                on_key(msg.key);
                break;
            case msg_type::touch :
                on_touch(msg.touch);
                break;
            }
        });
    }
    
    void android_window::on_surface_changed(const surface_data& data) {
        java_env je;
        surface_ = android_surface(je.env(), data.surface);
        // TODO: init opengles
    }
     
    void android_window::on_key(const key& data) {
    }

    void android_window::on_touch(const touch& data) {
    }
    
    void android_window::on_orientation_changed(const orientation& data) {
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
    extern "C" JNIEXPORT void JNICALL Java_enduro2d_engine_E2DNativeLib_create (JNIEnv* env, jobject, jobject activity) {
        auto& inst = java_interface::instance();
        inst.activity.set_activity(activity);
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
        java_interface::instance().window.push_msg({
            android_window::msg_type::surface_changed,
            android_window::surface_data(surface, w, h)
        });
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
#endif
