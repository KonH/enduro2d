/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "window.hpp"

#if defined(E2D_WINDOW_MODE) && E2D_WINDOW_MODE == E2D_WINDOW_MODE_NATIVE_ANDROID

#include <enduro2d/core/platform_impl/platform_android.hpp>
#include <enduro2d/utils/java.hpp>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <android/keycodes.h>
#include <EGL/egl.h>
#include <deque>

namespace
{
    using namespace e2d;
    
    //
    // message_queue
    //

    class message_queue {
    public:
        message_queue();
        size_t process();
        void push(std::function<void()>&& fn);
    private:
        std::mutex lock_;
        std::deque<std::function<void()>> queue_;
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
        void create_surface(ANativeWindow* window);
        void destroy_surface() noexcept;
        void bind_context() const noexcept;
        void swap_buffers() const noexcept;
        [[nodiscard]] bool has_context() const noexcept;
        [[nodiscard]] bool has_surface() const noexcept;
        [[nodiscard]] v2u framebuffer_size() const noexcept;
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
    // message_queue
    //
    
    message_queue::message_queue() {
        std::unique_lock<std::mutex> guard(lock_);
    }
    
    size_t message_queue::process() {
        size_t count = 0;
        for (;;) {
            std::function<void ()> fn;
            {
                std::unique_lock<std::mutex> guard(lock_);
                if ( queue_.empty() )
                    break;
                fn = std::move(queue_.front());
                queue_.pop_front();
            }
            fn();
            ++count;
        }
        return count;
    }
    
    void message_queue::push(std::function<void ()>&& fn) {
        std::unique_lock<std::mutex> guard(lock_);
        queue_.push_back(std::move(fn));
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
        // context & window should be destroyed
        E2D_ASSERT(context_ == EGL_NO_CONTEXT);
        E2D_ASSERT(!window_);
    }

    void android_surface::create_context(v4i rgba_size, i32 depth, i32 stencil, i32 samples) {
        E2D_ASSERT(context_ == EGL_NO_CONTEXT);
        EGL_CHECK_CODE(debug_, display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY));
        if ( display_ == EGL_NO_DISPLAY ) {
            throw std::runtime_error("failed to get EGL display");
        }
        EGLint maj_ver, min_ver;
        EGLBoolean ok;
        EGL_CHECK_CODE(debug_, ok = eglInitialize(display_, &maj_ver, &min_ver));
        if ( ok != EGL_TRUE ) {
            throw std::runtime_error("failed to initialize EGL");
        }
        egl_version_ = maj_ver * 100 + min_ver * 10;
        EGL_CHECK_CODE(debug_, eglBindAPI(EGL_OPENGL_ES_API));

        const EGLint required_config[] = {
            EGL_SURFACE_TYPE,     EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE,  EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };
        EGLConfig configs[1024];
        EGLint num_configs = 0;
        EGL_CHECK_CODE(debug_, ok = eglGetConfigs(display_, configs, std::size(configs), &num_configs));
        if ( ok != EGL_TRUE )
            throw std::runtime_error("failed to get EGL display configs");

        EGL_CHECK_CODE(debug_, ok = eglChooseConfig(display_, required_config, configs, std::size(configs), &num_configs));
        if ( ok != EGL_TRUE || num_configs == 0 ) {
            throw std::runtime_error("failed to choose EGL display config");
        }
        const auto get_attrib = [this] (EGLConfig cfg, EGLint attrib) {
            EGLint result = 0;
            EGLBoolean ok;
            EGL_CHECK_CODE(debug_, ok = eglGetConfigAttrib(display_, cfg, attrib, &result));
            return ok == EGL_TRUE ? result : 0;
        };
        config_ = nullptr;
        EGLint best_match_rgb = -1;
        EGLint best_match_rgb_d = -1;
        EGLint best_match_rgb_ds = -1;
        EGLint best_match_rgb_samp = -1;
        for ( EGLint i = 0; i < num_configs; ++i ) {
            EGLint d = get_attrib(configs[i], EGL_DEPTH_SIZE);
            EGLint s = get_attrib(configs[i], EGL_STENCIL_SIZE);
            EGLint r = get_attrib(configs[i], EGL_RED_SIZE);
            EGLint g = get_attrib(configs[i], EGL_GREEN_SIZE);
            EGLint b = get_attrib(configs[i], EGL_BLUE_SIZE);
            EGLint a = get_attrib(configs[i], EGL_ALPHA_SIZE);
            EGLint samp = get_attrib(configs[i], EGL_SAMPLES);
            bool rgb_match = v3i(rgba_size) <= v3i(r,g,b);
            bool depth_match = (!depth == !d);
            bool stencil_match = (!stencil == !s);
            bool samp_match = ((samples > 1) == (samp > 1));

            if ( rgba_size == v4i(r,g,b,a) && depth == d && stencil == s && samples == samp ) {
                config_ = configs[i];
            }
            if ( rgb_match ) {
                best_match_rgb = i;
            }
            if ( rgb_match && depth_match ) {
                best_match_rgb_d = i;
            }
            if ( rgb_match && depth_match && stencil_match ) {
                best_match_rgb_ds = i;
            }
            if ( rgb_match && depth_match && stencil_match && samp_match ) {
                best_match_rgb_samp = i;
            }
        }
        if ( !config_ && best_match_rgb_samp >= 0 ) {
            config_ = configs[best_match_rgb_samp];
        }
        if ( !config_ && best_match_rgb_ds >= 0 ) {
            config_ = configs[best_match_rgb_ds];
        }
        if ( !config_ && best_match_rgb_d >= 0 ) {
            config_ = configs[best_match_rgb_d];
        }
        if ( !config_ && best_match_rgb >= 0 ) {
            config_ = configs[best_match_rgb];
        }
        if ( !config_ ) {
            config_ = configs[0];
        }
        const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        EGL_CHECK_CODE(debug_, context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attribs));
        if ( context_ == EGL_NO_CONTEXT ) {
            throw std::runtime_error("failed to create EGL context");
        }
        debug_.trace("ANDROID: selected EGL surface config:\n"
            "--> EGL_DEPTH_SIZE: %0\n"
            "--> EGL_STENCIL_SIZE: %1\n"
            "--> EGL_RED_SIZE: %2\n"
            "--> EGL_GREEN_SIZE: %3\n"
            "--> EGL_BLUE_SIZE: %4\n"
            "--> EGL_ALPHA_SIZE: %5\n"
            "--> EGL_SAMPLES: %6",
            get_attrib(config_, EGL_DEPTH_SIZE),
            get_attrib(config_, EGL_STENCIL_SIZE),
            get_attrib(config_, EGL_RED_SIZE),
            get_attrib(config_, EGL_GREEN_SIZE),
            get_attrib(config_, EGL_BLUE_SIZE),
            get_attrib(config_, EGL_ALPHA_SIZE),
            get_attrib(config_, EGL_SAMPLES));
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

    void android_surface::create_surface(ANativeWindow* window) {
        if ( context_ == EGL_NO_CONTEXT ) {
            throw std::runtime_error("can't create surface without EGL context");
        }
        destroy_surface();
        window_ = window;
        
        EGLint format = 0;
        EGL_CHECK_CODE(debug_, eglGetConfigAttrib(display_, config_, EGL_NATIVE_VISUAL_ID, &format));

        if ( ANativeWindow_setBuffersGeometry(window_, 0, 0, format) ) {
            throw std::runtime_error("failed to set pixel format to native window");
        }
        const EGLint surface_attribs[] = {
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE
        };
        EGL_CHECK_CODE(debug_,  surface_ = eglCreateWindowSurface(display_, config_, window_, surface_attribs));
        if ( surface_ == EGL_NO_SURFACE ) {
            throw std::runtime_error("failed to create window surface");
        }
        EGLBoolean ok;
        EGL_CHECK_CODE(debug_, ok = eglMakeCurrent(display_, surface_, surface_, context_));
        if ( ok != EGL_TRUE ) {
            throw std::runtime_error("failed to make EGL context current");
        }
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

    void android_surface::bind_context() const noexcept {
        EGL_CHECK_CODE(debug_, eglMakeCurrent(display_, surface_, surface_, context_));
    }

    void android_surface::swap_buffers() const noexcept {
        E2D_ASSERT(display_ != EGL_NO_DISPLAY);
        E2D_ASSERT(surface_ != EGL_NO_SURFACE);
        E2D_ASSERT(context_ != EGL_NO_CONTEXT && context_ == eglGetCurrentContext());
        EGL_CHECK_CODE(debug_, eglSwapBuffers(display_, surface_));
    }
    
    bool android_surface::has_context() const noexcept {
        return context_ != EGL_NO_CONTEXT;
    }
    
    bool android_surface::has_surface() const noexcept {
        return surface_ != EGL_NO_SURFACE && window_ != nullptr;
    }

    v2u android_surface::framebuffer_size() const noexcept {
        E2D_ASSERT(display_ != EGL_NO_DISPLAY);
        E2D_ASSERT(surface_ != EGL_NO_SURFACE);
        v2u result;
        EGL_CHECK_CODE(debug_, eglQuerySurface(display_, surface_, EGL_WIDTH, reinterpret_cast<EGLint*>(&result.x)));
        EGL_CHECK_CODE(debug_, eglQuerySurface(display_, surface_, EGL_HEIGHT, reinterpret_cast<EGLint*>(&result.y)));
        return result;
    }
    
    //
    // convert_*
    //

    keyboard_key convert_android_keyboard_key(int code) noexcept {
        switch ( code ) {
            case AKEYCODE_0 : return keyboard_key::_0;
            case AKEYCODE_1 : return keyboard_key::_1;
            case AKEYCODE_2 : return keyboard_key::_2;
            case AKEYCODE_3 : return keyboard_key::_3;
            case AKEYCODE_4 : return keyboard_key::_4;
            case AKEYCODE_5 : return keyboard_key::_5;
            case AKEYCODE_6 : return keyboard_key::_6;
            case AKEYCODE_7 : return keyboard_key::_7;
            case AKEYCODE_8 : return keyboard_key::_8;
            case AKEYCODE_9 : return keyboard_key::_9;
                
            case AKEYCODE_A : return keyboard_key::a;
            case AKEYCODE_B : return keyboard_key::b;
            case AKEYCODE_C : return keyboard_key::c;
            case AKEYCODE_D : return keyboard_key::d;
            case AKEYCODE_E : return keyboard_key::e;
            case AKEYCODE_F : return keyboard_key::f;
            case AKEYCODE_G : return keyboard_key::g;
            case AKEYCODE_H : return keyboard_key::h;
            case AKEYCODE_I : return keyboard_key::i;
            case AKEYCODE_J : return keyboard_key::j;
            case AKEYCODE_K : return keyboard_key::k;
            case AKEYCODE_L : return keyboard_key::l;
            case AKEYCODE_M : return keyboard_key::m;
            case AKEYCODE_N : return keyboard_key::n;
            case AKEYCODE_O : return keyboard_key::o;
            case AKEYCODE_P : return keyboard_key::p;
            case AKEYCODE_Q : return keyboard_key::q;
            case AKEYCODE_R : return keyboard_key::r;
            case AKEYCODE_S : return keyboard_key::s;
            case AKEYCODE_T : return keyboard_key::t;
            case AKEYCODE_U : return keyboard_key::u;
            case AKEYCODE_V : return keyboard_key::v;
            case AKEYCODE_W : return keyboard_key::w;
            case AKEYCODE_X : return keyboard_key::x;
            case AKEYCODE_Y : return keyboard_key::y;
            case AKEYCODE_Z : return keyboard_key::z;
                
            case AKEYCODE_MINUS : return keyboard_key::minus;
            case AKEYCODE_EQUALS : return keyboard_key::equal;
            case AKEYCODE_GRAVE : return keyboard_key::grave_accent;
            case AKEYCODE_BACKSLASH : return keyboard_key::backslash;
            case AKEYCODE_SLASH : return keyboard_key::slash;
            case AKEYCODE_SEMICOLON : return keyboard_key::semicolon;
            case AKEYCODE_APOSTROPHE : return keyboard_key::apostrophe;
            case AKEYCODE_COMMA : return keyboard_key::comma;
            case AKEYCODE_PERIOD : return keyboard_key::period;
            case AKEYCODE_TAB : return keyboard_key::tab;
            case AKEYCODE_ENTER : return keyboard_key::enter;
            case AKEYCODE_SPACE : return keyboard_key::space;
            case AKEYCODE_LEFT_BRACKET : return keyboard_key::lbracket;
            case AKEYCODE_RIGHT_BRACKET : return keyboard_key::rbracket;
            case AKEYCODE_SHIFT_LEFT : return keyboard_key::lshift;
            case AKEYCODE_SHIFT_RIGHT : return keyboard_key::rshift;
            case AKEYCODE_ALT_LEFT : return keyboard_key::lalt;
            case AKEYCODE_ALT_RIGHT : return keyboard_key::ralt;
            case AKEYCODE_PAGE_UP : return keyboard_key::page_up;
            case AKEYCODE_PAGE_DOWN : return keyboard_key::page_down;
            case AKEYCODE_HOME : return keyboard_key::home;
            case AKEYCODE_DEL : return keyboard_key::backspace;
            case AKEYCODE_FORWARD_DEL : return keyboard_key::del;
            case AKEYCODE_MENU : return keyboard_key::menu;
            case AKEYCODE_CAPS_LOCK : return keyboard_key::caps_lock;
            case AKEYCODE_INSERT : return keyboard_key::insert;
            case AKEYCODE_BREAK : return keyboard_key::pause;
            case AKEYCODE_SCROLL_LOCK : return keyboard_key::scroll_lock;
            case AKEYCODE_CTRL_LEFT : return keyboard_key::lcontrol;
            case AKEYCODE_CTRL_RIGHT : return keyboard_key::rcontrol;
            case AKEYCODE_ESCAPE : return keyboard_key::escape;
            case AKEYCODE_BACK : return keyboard_key::escape;
            case AKEYCODE_SOFT_LEFT : return keyboard_key::lsuper;
            case AKEYCODE_SOFT_RIGHT : return keyboard_key::rsuper;
                
            case AKEYCODE_DPAD_LEFT : return keyboard_key::left;
            case AKEYCODE_DPAD_UP : return keyboard_key::up;
            case AKEYCODE_DPAD_RIGHT : return keyboard_key::right;
            case AKEYCODE_DPAD_DOWN : return keyboard_key::down;

            case AKEYCODE_F1 : return keyboard_key::f1;
            case AKEYCODE_F2 : return keyboard_key::f2;
            case AKEYCODE_F3 : return keyboard_key::f3;
            case AKEYCODE_F4 : return keyboard_key::f4;
            case AKEYCODE_F5 : return keyboard_key::f5;
            case AKEYCODE_F6 : return keyboard_key::f6;
            case AKEYCODE_F7 : return keyboard_key::f7;
            case AKEYCODE_F8 : return keyboard_key::f8;
            case AKEYCODE_F9 : return keyboard_key::f9;
            case AKEYCODE_F10 : return keyboard_key::f10;
            case AKEYCODE_F11 : return keyboard_key::f11;
            case AKEYCODE_F12 : return keyboard_key::f12;

            case AKEYCODE_NUMPAD_0 : return keyboard_key::kp_0;
            case AKEYCODE_NUMPAD_1 : return keyboard_key::kp_1;
            case AKEYCODE_NUMPAD_2 : return keyboard_key::kp_2;
            case AKEYCODE_NUMPAD_3 : return keyboard_key::kp_3;
            case AKEYCODE_NUMPAD_4 : return keyboard_key::kp_4;
            case AKEYCODE_NUMPAD_5 : return keyboard_key::kp_5;
            case AKEYCODE_NUMPAD_6 : return keyboard_key::kp_6;
            case AKEYCODE_NUMPAD_7 : return keyboard_key::kp_7;
            case AKEYCODE_NUMPAD_8 : return keyboard_key::kp_8;
            case AKEYCODE_NUMPAD_9 : return keyboard_key::kp_9;
            case AKEYCODE_NUM_LOCK : return keyboard_key::kp_num_lock;
            case AKEYCODE_NUMPAD_DIVIDE : return keyboard_key::kp_divide;
            case AKEYCODE_NUMPAD_MULTIPLY : return keyboard_key::kp_multiply;
            case AKEYCODE_NUMPAD_SUBTRACT : return keyboard_key::kp_subtract;
            case AKEYCODE_NUMPAD_ENTER : return keyboard_key::kp_enter;
            case AKEYCODE_NUMPAD_DOT : return keyboard_key::kp_decimal;
            case AKEYCODE_NUMPAD_COMMA : return keyboard_key::kp_decimal;
            case AKEYCODE_NUMPAD_ADD : return keyboard_key::kp_add;
            case AKEYCODE_NUMPAD_EQUALS : return keyboard_key::kp_equal;

            default :
                return keyboard_key::unknown;
        }
    }

    keyboard_key_action convert_android_keyboard_key_act(int action) noexcept {
        // from KeyEvents.java
        constexpr int ACTION_DOWN = 0;
        constexpr int ACTION_UP = 1;
        constexpr int ACTION_MULTIPLE = 2;
        switch ( action ) {
            case ACTION_DOWN:
                return keyboard_key_action::press;
            case ACTION_MULTIPLE:
                return keyboard_key_action::repeat;
            case ACTION_UP:
                return keyboard_key_action::release;
            default:
                return keyboard_key_action::unknown;
        }
    }
}

namespace e2d
{
    //
    // e2d_native_lib::activity_interface (UI thread)
    //

    class e2d_native_lib::activity_interface {
    public:
        // see https://developer.android.com/reference/android/R.attr.html#screenOrientation
        enum class orientation : u32 {
            unknown = 0xffffffff,
            full_sensor = 0xa,
            full_user = 0xd,
            landscape = 0x0,
            locked = 0xe,
            no_sensor = 0x5,
            portrait = 0x1,
            reverse_landscape = 0x8,
            reverse_portrait = 0x9,
            sensor = 0x4,
            sensor_landscape = 0x6,
            sensor_portrait = 0x7,
            user = 0x2,
            user_landscape = 0xb,
            user_portrait = 0xc,
        };
    public:
        explicit activity_interface(jobject);
        void release();
        bool process_messages();
        [[nodiscard]] bool is_current_thread() const noexcept;
    public:
        void set_orientation(orientation);
        void show_toast(str_view);
        void set_title(str_view);
        void close();
    public:
        java_obj main_activity_;
        java_method<void ()> finish_activity_;
        java_method<void (jstring)> set_activity_title_;
        java_method<void (jstring, jboolean)> show_toast_;
        java_method<void (jint)> set_orientation_;
        java_method<jboolean ()> has_network_;
        message_queue messages_;
        std::thread::id thread_id_ = std::this_thread::get_id();
    };
    
    //
    // e2d_native_lib::renderer_interface
    //

    class e2d_native_lib::renderer_interface {
    public:
        struct touch {
            struct pointer {
                u32 id;
                f32 x, y;
                f32 pressure;
            };
            i32 action;
            i32 pointer_count;
            std::array<pointer, 8> pointers;
        };
        
        enum class orientation {
            unknown,
            _0,
            _90,
            _180,
            _270
        };

        using event_listener_uptr = window::event_listener_uptr;
        using listeners_t = vector<event_listener_uptr>;
    public:
        renderer_interface() noexcept;
        void release();
        bool process_messages();
        [[nodiscard]] bool is_current_thread() const noexcept;
        [[nodiscard]] const android_surface& surface() const noexcept;
    
        template < typename F, typename... Args >
        void for_all_listeners(const F& f, const Args&... args) noexcept;
    public:
        void on_surface_changed(ANativeWindow* wnd);
        void on_destroy();
        void on_key(i32 code, i32 action);
        void on_touch(const touch&);
        void set_display_info(int w, int h, int ppi);
        void set_orientation(int value);
    private:
        void render_loop_() noexcept;
    public:
        std::recursive_mutex rmutex;
        listeners_t listeners;
        v2f physics_size; // in millimeters
        v2u real_size; // in pixels
        v2u virtual_size; // in virtual pixels
        v2u framebuffer_size; // in pixels
        str title;
        bool fullscreen = false;
        bool enabled = false;
        bool visible = true;
        bool focused = true;
        bool should_close = false;
    private:
        android_surface surface_;
        message_queue messages_;
        std::thread thread_;
        std::atomic<bool> exit_loop_ = false;
        std::atomic<bool> surface_destoyed_ = false;
        u32 last_touch_id_ = ~0u;
        v2u display_size_;
        int display_ppi_ = 96;
        orientation display_orientation_ = orientation::unknown;
    };
    
    //
    // e2d_native_lib::activity_interface
    //
    
    e2d_native_lib::activity_interface::activity_interface(jobject obj) {
        main_activity_ = java_obj(obj);
        finish_activity_ = main_activity_.method<void ()>("finish");
        set_activity_title_ = main_activity_.method<void (jstring)>("setActivityTitle");
        show_toast_ = main_activity_.method<void (jstring, jboolean)>("showToast");
        set_orientation_ = main_activity_.method<void (jint)>("setScreenOrientation");
        has_network_ = main_activity_.method<jboolean ()>("isNetworkConnected");
    }
    
    void e2d_native_lib::activity_interface::release() {
        main_activity_ = {};
        finish_activity_ = {};
        set_activity_title_ = {};
        show_toast_ = {};
        set_orientation_ = {};
        has_network_ = {};
    }

    bool e2d_native_lib::activity_interface::process_messages() {
        E2D_ASSERT(is_current_thread());
        return messages_.process() > 0;
    }

    void e2d_native_lib::activity_interface::set_orientation(orientation value) {
        messages_.push([this, value]() {
            if ( !main_activity_ ) {
                return;
            }
            set_orientation_(jint(value));
        });
    }

    void e2d_native_lib::activity_interface::show_toast(str_view value) {
        messages_.push([this, msg = str(value)] () {
            if ( !main_activity_ ) {
                return;
            }
            show_toast_(java_string(msg), false);
        });
    }

    void e2d_native_lib::activity_interface::set_title(str_view value) {
        messages_.push([this, title = str(value)] () {
            if ( !main_activity_ ) {
                return;
            }
            set_activity_title_(java_string(title));
        });
    }

    void e2d_native_lib::activity_interface::close() {
        messages_.push([this] () {
            if ( !main_activity_ ) {
                return;
            }
            finish_activity_();
            release();
        });
    }
    
    bool e2d_native_lib::activity_interface::is_current_thread() const noexcept {
        return std::this_thread::get_id() == thread_id_;
    }
    
    //
    // renderer_interface
    //
    
    e2d_native_lib::renderer_interface::renderer_interface() noexcept
    : surface_(the<debug>())
    , thread_([this]() { render_loop_(); }) {
    }
    
    void e2d_native_lib::renderer_interface::release() {
        exit_loop_.store(true);
        thread_.join();
    }

    void e2d_native_lib::renderer_interface::render_loop_() noexcept {
        E2D_ASSERT(is_current_thread());
        bool main_was_called = false;

        for (; !exit_loop_.load(std::memory_order_relaxed); ) {
            try {
                process_messages();
            } catch(const std::exception &e) {
                __android_log_print(ANDROID_LOG_ERROR, "enduro2d", "renderer_interface::render_loop_ exception: %s\n", e.what());
            }
            if ( !main_was_called && surface_.has_surface() ) {
                main_was_called = true;
                char arg[1] = {};
                char* argv[] = {arg};
                int res = e2d_main(std::size(argv), argv);
                __android_log_print(ANDROID_LOG_ERROR, "enduro2d", "e2d_main result %i\n", res);
                exit_loop_.store(true); // temp ?
            }
        }
        
        e2d_native_lib::state().activity().close();
    }

    bool e2d_native_lib::renderer_interface::process_messages() {
        std::unique_lock<std::recursive_mutex> guard_(this->rmutex);
        return messages_.process() > 0;
    }
    
    void e2d_native_lib::renderer_interface::on_surface_changed(ANativeWindow* wnd) {
        using time_point = std::chrono::steady_clock::time_point;
        using millis = std::chrono::milliseconds;

        constexpr millis timeout = millis(2000);
        const time_point start = time_point::clock::now();
        surface_destoyed_ = false;

        messages_.push([this, wnd] () {
            if ( wnd ) {
                if ( !surface_.has_context() ) {
                    surface_.create_context(v4i(8,8,8,0), 16, 0, 0);
                }
                surface_.create_surface(wnd);
                framebuffer_size = surface_.framebuffer_size();
                real_size = surface_.framebuffer_size();
            } else {
                surface_.destroy_surface();
                framebuffer_size = v2u(0,0);
                real_size = v2u(0,0);
                surface_destoyed_ = true;
            }
            enabled = surface_.has_surface();
        });

        // if surface destroyed then block UI thread until rendering thread complete frame
        if ( !wnd ) {
            for (; !surface_destoyed_; ) {
                millis dt = std::chrono::duration_cast<millis>(time_point::clock::now() - start);
                if ( dt > timeout ) {
                    __android_log_write(ANDROID_LOG_ERROR, "enduro2d", "ANDROID: time is out when waiting render thread");
                    return;
                }
            }
        }
    }

    void e2d_native_lib::renderer_interface::on_destroy() {
        messages_.push([this] () {
            surface_.destroy_context();
            exit_loop_.store(true, std::memory_order_relaxed);
            should_close = true;
        });
    }

    void e2d_native_lib::renderer_interface::on_key(i32 code, i32 action) {
        messages_.push([this, code, action] () {
            for_all_listeners(
                &window::event_listener::on_keyboard_key,
                convert_android_keyboard_key(code),
                code,
                convert_android_keyboard_key_act(action));
        });
    }

    void e2d_native_lib::renderer_interface::on_touch(const touch& data) {
        messages_.push([this, data] () {
            // from MotionEvent.java
            constexpr int action_down = 0;
            constexpr int action_up = 1;
            constexpr int action_move = 2;
            constexpr int action_cancel = 3;
            constexpr int action_outside = 4;
        
            for ( int i = 0; i < data.pointer_count; ++i ) {
                auto& ptr = data.pointers[i];
                if ( last_touch_id_ == ptr.id ) {
                    for_all_listeners(
                        &window::event_listener::on_move_cursor,
                        v2f(ptr.x, ptr.y));
                    break;
                }
            }
            auto& ptr = data.pointers.front();
            switch ( data.action ) {
                case action_down :
                    if ( last_touch_id_ == ~0u ) {
                        last_touch_id_ = ptr.id;
                        for_all_listeners(
                            &window::event_listener::on_mouse_button,
                            mouse_button(last_touch_id_),
                            mouse_button_action::press);
                    }
                    break;
                case action_up :
                case action_cancel :
                case action_outside :
                    if ( last_touch_id_ == ptr.id ) {
                        for_all_listeners(
                            &window::event_listener::on_mouse_button,
                            mouse_button(last_touch_id_),
                            mouse_button_action::release);
                        last_touch_id_ = ~0u;
                    }
                    break;
            }
        });
    }

    void e2d_native_lib::renderer_interface::set_display_info(int w, int h, int ppi) {
        messages_.push([this, w, h, ppi]() {
            display_size_ = v2u(w, h);
            display_ppi_ = ppi;
            const float inch_to_mm = 25.4f;
            physics_size = display_size_.cast_to<f32>() / f32(ppi) * inch_to_mm;
        });
    }
        
    void e2d_native_lib::renderer_interface::set_orientation(int value) {
        messages_.push([this, value]() {
            // from Surface.java
            constexpr int rotation_0 = 0;
            constexpr int rotation_90 = 1;
            constexpr int rotation_180 = 2;
            constexpr int rotation_270 = 3;
            switch ( value ) {
                case rotation_0 : display_orientation_ = orientation::_0; break;
                case rotation_90 : display_orientation_ = orientation::_90; break;
                case rotation_180 : display_orientation_ = orientation::_180; break;
                case rotation_270 : display_orientation_ = orientation::_270; break;
                default : display_orientation_ = orientation::unknown; break;
            }
        });
    }

    template < typename F, typename... Args >
    void e2d_native_lib::renderer_interface::for_all_listeners(const F& f, const Args&... args) noexcept {
        std::lock_guard<std::recursive_mutex> guard(rmutex);
        for ( const event_listener_uptr& listener : listeners ) {
            if ( listener ) {
                stdex::invoke(f, listener.get(), args...);
            }
        }
    }
    
    const android_surface& e2d_native_lib::renderer_interface::surface() const noexcept {
        E2D_ASSERT(is_current_thread());
        return surface_;
    }

    bool e2d_native_lib::renderer_interface::is_current_thread() const noexcept {
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
        state(const v2u& size) noexcept;
        ~state() noexcept = default;
        auto& native_window() noexcept;
    };
    
    auto& window::state::native_window() noexcept {
        return e2d_native_lib::state().renderer();
    }
    
    window::state::state(const v2u& size) noexcept {
        auto& wnd = native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        wnd.virtual_size = size;
    }

    //
    // window
    //

    window::window(const v2u& size, str_view title, bool vsync, bool fullscreen)
    : state_(new state(size)) {
        E2D_UNUSED(title, vsync, fullscreen);
    }

    window::~window() noexcept = default;

    void window::hide() noexcept {
        // TODO
    }

    void window::show() noexcept {
        // TODO
    }

    void window::restore() noexcept {
        // TODO
    }

    void window::minimize() noexcept {
        // TODO
    }

    bool window::enabled() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.enabled;
    }

    bool window::visible() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.visible;
    }

    bool window::focused() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.focused;
    }

    bool window::minimized() const noexcept {
        // TODO
        return false;
    }

    bool window::fullscreen() const noexcept {
        // TODO
        return true;
    }

    bool window::toggle_fullscreen(bool yesno) noexcept {
        // TODO
        return true;
    }

    void window::hide_cursor() noexcept {
        // TODO
    }

    void window::show_cursor() noexcept {
        // TODO
    }

    bool window::is_cursor_hidden() const noexcept {
        // TODO
        return true;
    }

    v2u window::real_size() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.real_size;
    }

    v2u window::virtual_size() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.virtual_size;
    }

    v2u window::framebuffer_size() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.framebuffer_size;
    }

    const str& window::title() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.title; // TODO: this is not a thread safe code
    }

    void window::set_title(str_view title) {
        e2d_native_lib::state().activity().set_title(title);
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        wnd.title = title;
    }

    bool window::should_close() const noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        return wnd.should_close;
    }

    void window::set_should_close(bool yesno) noexcept {
        e2d_native_lib::state().activity().close();
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        wnd.should_close = true;
    }

    void window::bind_context() noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        wnd.surface().bind_context();
    }

    void window::swap_buffers() noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        wnd.surface().swap_buffers();
    }

    bool window::poll_events() noexcept {
        return e2d_native_lib::state().renderer().process_messages();
    }

    window::event_listener& window::register_event_listener(event_listener_uptr listener) {
        E2D_ASSERT(listener);
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        wnd.listeners.push_back(std::move(listener));
        return *wnd.listeners.back(); // TODO: this is not a thread safe code
    }

    void window::unregister_event_listener(const event_listener& listener) noexcept {
        auto& wnd = state_->native_window();
        std::lock_guard<std::recursive_mutex> guard(wnd.rmutex);
        for ( auto iter = wnd.listeners.begin(); iter != wnd.listeners.end(); ) {
            if ( iter->get() == &listener ) {
                iter = wnd.listeners.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

namespace e2d
{
    //
    // e2d_native_lib::internal_state
    //

    void e2d_native_lib::internal_state::destroy_activity_(activity_interface* ptr) {
        static_assert(sizeof(*ptr) > 0, "activity_interface must be defined");
        delete ptr;
    }
    
    void e2d_native_lib::internal_state::destroy_renderer_(renderer_interface* ptr) {
        static_assert(sizeof(*ptr) > 0, "renderer_interface must be defined");
        delete ptr;
    }

    //
    // e2d_native_lib
    //

    void JNICALL e2d_native_lib::create_window(JNIEnv* env, jclass, jobject activity) noexcept {
        try {
            state().activity_.reset(new activity_interface(activity));
            state().renderer_.reset(new renderer_interface());
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::destroy_window(JNIEnv* env, jclass) noexcept {
        try {
            state().activity().release();
            state().renderer().on_destroy();
            state().renderer().release();
            state().activity().process_messages();
            state().renderer_.reset();
            state().activity_.reset();
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::start(JNIEnv* env, jclass) noexcept {
        try {
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::stop(JNIEnv* env, jclass) noexcept {
        try {
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::pause(JNIEnv* env, jclass) noexcept {
        try {
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::resume(JNIEnv* env, jclass) noexcept {
        try {
            state().activity().set_activity_title_(java_string("exception test"));
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::surface_changed(JNIEnv* env, jclass, jobject surface) noexcept {
        try {
            state().renderer().on_surface_changed(ANativeWindow_fromSurface(env, surface));
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::surface_destroyed(JNIEnv* env, jclass) noexcept {
        try {
            state().renderer().on_surface_changed(nullptr);
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::visibility_changed(JNIEnv* env, jclass) noexcept {
        try {

        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::orientation_changed(JNIEnv* env, jclass, jint value) noexcept {
        try {
            state().renderer().set_orientation(value);
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::tick(JNIEnv* env, jclass) noexcept {
        try {
            state().activity().process_messages();
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::on_key(JNIEnv* env, jclass, jint keycode, jint action) noexcept {
        try {
            state().renderer().on_key(keycode, action);
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::on_touch(JNIEnv* env, jclass, jint action, jint num_pointers, jfloatArray touch_data_array) noexcept {
        try {
            renderer_interface::touch touch;
            touch.action = action;
            touch.pointer_count = num_pointers;
            java_array<jfloat> touch_data(touch_data_array);
            for ( int i = 0, j = 0; i < num_pointers; ++i ) {
                touch.pointers[i].id = u32(touch_data[j++]);
                touch.pointers[i].x = touch_data[j++];
                touch.pointers[i].y = touch_data[j++];
                touch.pointers[i].pressure = touch_data[j++];
            }
            state().renderer().on_touch(touch);
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }

    void JNICALL e2d_native_lib::set_display_info(JNIEnv* env, jclass, jint w, jint h, jint ppi) noexcept {
        try {
            state().renderer().set_display_info(w, h, ppi);
        } catch(const std::exception& e) {
            check_exceptions_(env, e);
        }
    }
}

#endif
