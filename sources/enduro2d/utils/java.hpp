/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include <enduro2d/utils/_utils.hpp>

#if defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_ANDROID
#  define E2D_JNI
#endif

#ifdef E2D_JNI

#include <jni.h>

namespace e2d
{

    //
    // java_env
    //

    class java_env final {
    public:
        java_env() noexcept;
        explicit java_env(JNIEnv *env) noexcept;
        ~java_env() noexcept;
        void attach();
        void detach();
        void throw_exception(str_view msg) const;
        void exception_clear() const noexcept;
        [[nodiscard]] bool has_exception() const noexcept;
        [[nodiscard]] JNIEnv* env() const noexcept;
    private:
        JNIEnv* jni_env_ = nullptr;
        bool must_be_detached_ = false;
    };

    namespace detail
    {
        //
        // java_vm
        //

        class java_vm {
        public:
            [[nodiscard]] static JavaVM* get() noexcept;
            static void set(JavaVM*) noexcept;
        private:
            static JavaVM*& get_jvm_() noexcept;
        };

        //
        // jni_type_name
        //

        template < typename T >
        struct jni_type_name;

        #define JNI_TYPE_NAME(type_name, signature) \
            template <> \
            struct jni_type_name<type_name> { \
                using type = type_name; \
                static void append(str& s) { s += signature; } \
            }
        JNI_TYPE_NAME(void, 'V');
        JNI_TYPE_NAME(jboolean, 'Z');
        JNI_TYPE_NAME(jbyte, 'B');
        JNI_TYPE_NAME(jchar, 'C');
        JNI_TYPE_NAME(jshort, 'S');
        JNI_TYPE_NAME(jint, 'I');
        JNI_TYPE_NAME(jlong, 'J');
        JNI_TYPE_NAME(jfloat, 'F');
        JNI_TYPE_NAME(jdouble, 'D');
        JNI_TYPE_NAME(jstring, "Ljava/lang/String;");
        JNI_TYPE_NAME(jobject, "Ljava/lang/Object;");
        JNI_TYPE_NAME(jthrowable, "Ljava/lang/Throwable;");
        JNI_TYPE_NAME(jbyteArray, "[B");
        JNI_TYPE_NAME(jcharArray, "[C");
        JNI_TYPE_NAME(jshortArray, "[S");
        JNI_TYPE_NAME(jintArray, "[I");
        JNI_TYPE_NAME(jlongArray, "[J");
        JNI_TYPE_NAME(jfloatArray, "[F");
        JNI_TYPE_NAME(jdoubleArray, "[D");
        #undef JNI_TYPE_NAME
    
        template < typename T >
        struct jni_type_name<T*> {
            using type = T*;
            static void append(str& s) { s += '[';  jni_type_name<T>::append(s); }
        };

        //
        // java_method_result
        //

        template < typename T >
        struct [[nodiscard]] java_method_result {
            T value_;
            java_method_result(T) noexcept;
            [[nodiscard]] operator T() const noexcept;
        };
    
        template < typename T >
        inline java_method_result<T>::java_method_result(T value) noexcept
        : value_(value) {
        }

        template < typename T >
        inline java_method_result<T>::operator T() const noexcept {
            return value_;
        }
    
        template <>
        struct java_method_result<void> {};

        //
        // java_method_sig
        //
        
        template < typename FN >
        class java_method_sig;
        
        template < typename Ret, typename ...Args >
        class java_method_sig<Ret (Args...)> final {
        public:
            java_method_sig() {
                signature_ += '(';
                if constexpr ( sizeof...(Args) > 0 ) {
                    append_args<Args...>();
                }
                signature_ += ')';
                jni_type_name<Ret>::append(signature_);
            }
            [[nodiscard]] const str& signature() const noexcept {
                return signature_;
            }
        private:
            template < typename Arg0, typename ...ArgsN >
            void append_args() {
                jni_type_name<Arg0>::append(signature_);
                if constexpr ( sizeof...(ArgsN) > 0 ) {
                    append_args<ArgsN...>();
                }
            }
        private:
            str signature_;
        };
    }
    
    //
    // java_array
    //

    template < typename T >
    class java_array;

    #define DEFINE_JAVE_ARRAY(type, jtype) \
        template <> \
        class java_array<type> { \
        public: \
            using value_type = type; \
            using jarray_t = type ## Array; \
        public: \
            java_array() noexcept = default; \
            \
            explicit java_array(jarray_t arr, bool read_only = false) noexcept \
            : jarray_(arr) \
            , read_only_(read_only) { \
                java_env je; \
                data_ = je.env()->Get ## jtype ## ArrayElements(jarray_, 0); \
                count_ = je.env()->GetArrayLength(jarray_); \
            } \
            \
            explicit java_array(const std::vector<type>& vec) noexcept \
            : from_native_code_(true) { \
                java_env je; \
                jarray_ = je.env()->New ## jtype ## Array(vec.size()); \
                data_ = je.env()->Get ## jtype ## ArrayElements(jarray_, nullptr); \
                count_ = je.env()->GetArrayLength(jarray_); \
                E2D_ASSERT(vec.size() == count_); \
                for ( size_t i = 0; i < count_; ++i ) { \
                    data_[i] = vec[i]; \
                } \
            } \
            \
            java_array(java_array&& other) noexcept \
            : from_native_code_(other.from_native_code_) { \
                std::swap(data_, other.data_); \
                std::swap(jarray_, other.jarray_); \
                std::swap(count_, other.count_); \
                std::swap(read_only_, other.read_only_); \
            } \
            \
            ~java_array() noexcept { \
                release_(); \
            } \
            \
            [[nodiscard]] const type* data() const noexcept { \
                return data_; \
            } \
            \
            /*[[nodiscard]] type* data() { \
                if ( read_only_ ) { \
                    throw std::runtime_exception("can not modify read-only memory!"); \
                } \
                return data_; \
            }*/ \
            \
            [[nodiscard]] const type& operator [] (size_t i) const noexcept { \
                E2D_ASSERT(i < count_); \
                return data_[i]; \
            } \
            \
            [[nodiscard]] size_t size() const noexcept { \
                return count_; \
            } \
            \
            [[nodiscard]] bool read_only() const noexcept { \
                return read_only_; \
            } \
            \
            [[nodiscard]] const type* begin() const noexcept { \
                return data_; \
            } \
            \
            [[nodiscard]] const type* end() const noexcept { \
                return data_ + count_; \
            } \
            \
            [[nodiscard]] jarray_t get() const noexcept { \
                return jarray_; \
            } \
            \
        private: \
            void release_() noexcept { \
                java_env je; \
                if ( from_native_code_ ) { \
                    if ( jarray_ ) { \
                        je.env()->DeleteLocalRef(jarray_); \
                    } \
                } else if ( data_ && jarray_ ) { \
                    je.env()->Release ## jtype ## ArrayElements(jarray_, data_, read_only_ ? JNI_ABORT : 0); \
                } \
                data_ = nullptr; \
                jarray_ = nullptr; \
                count_ = 0; \
                read_only_ = false; \
            } \
        private: \
            type* data_ = nullptr; \
            jarray_t jarray_ = nullptr; \
            size_t count_ = 0; \
            bool read_only_ = false; \
            bool from_native_code_ = false; \
        }
    DEFINE_JAVE_ARRAY(jbyte, Byte);
    DEFINE_JAVE_ARRAY(jchar, Char);
    DEFINE_JAVE_ARRAY(jshort, Short);
    DEFINE_JAVE_ARRAY(jint, Int);
    DEFINE_JAVE_ARRAY(jlong, Long);
    DEFINE_JAVE_ARRAY(jfloat, Float);
    DEFINE_JAVE_ARRAY(jdouble, Double);
    #undef DEFINE_JAVE_ARRAY

    //
    // java_string
    //

    class java_string {
    public:
        java_string() noexcept = default;
        explicit java_string(jstring) noexcept;
        explicit java_string(str_view) noexcept;
        java_string(java_string&&) noexcept;
        ~java_string() noexcept;
        [[nodiscard]] const char* data() const noexcept;
        [[nodiscard]] size_t length() const noexcept;
        [[nodiscard]] jstring get() const noexcept;
        operator str_view() const noexcept;
    private:
        void release_() noexcept;
    private:
        const char* data_ = nullptr;
        size_t length_ = 0;
        jstring jstr_ = nullptr;
        bool from_native_code_ = false;
    };

    template < typename FN >
    class java_static_method;

    template < typename FN >
    class java_method;

    //
    // java_class
    //

    class java_class final {
    public:
        explicit java_class(str_view);
        explicit java_class(jclass jc) noexcept;
        explicit java_class(const class java_obj&);
        template < typename T >
        explicit java_class(stdex::in_place_type_t<T>);
        java_class(java_class&&) noexcept;
        java_class(const java_class&) noexcept;
        java_class() noexcept = default;
        ~java_class() noexcept;
        java_class& operator = (const java_class&) noexcept;
        java_class& operator = (java_class&&) noexcept;
        [[nodiscard]] jclass get() const noexcept;
        template < typename FN >
        [[nodiscard]] java_static_method<FN> static_method(str_view name) const;
        [[nodiscard]] explicit operator bool() const noexcept;
        template < typename Ret, typename ...Args >
        void register_method(str_view name, Ret (JNICALL *fn)(JNIEnv*, jobject, Args...)) const;
        template < typename Ret, typename ...Args >
        void register_static_method(str_view name, Ret (JNICALL *fn)(JNIEnv*, jclass, Args...)) const;
    private:
        void set_(const java_env&, jclass) noexcept;
        void dec_ref_() noexcept;
    private:
        jclass class_ = nullptr;
    };
    
    template < typename T >
    inline java_class::java_class(stdex::in_place_type_t<T>) {
        java_env je;
        str class_name;
        detail::jni_type_name<T>::append(class_name);
        // remove first 'L'
        if ( class_name.length() > 0 and class_name.front() == 'L' ) {
            class_name = class_name.substr(1);
        }
        // remove last ';'
        if ( class_name.length() and class_name.back() == ';' ) {
            class_name.erase(class_name.end()-1);
        }
        jclass jc = je.env()->FindClass(class_name.data());
        if ( !jc ) {
            throw std::runtime_error("java class is not found");
        }
        set_(je, jc);
    }

    template < typename Ret, typename ...Args >
    void java_class::register_static_method(str_view name, Ret (JNICALL *fn)(JNIEnv*, jclass, Args...)) const {
        if ( !class_ ) {
            throw std::runtime_error("invalid java class");
        }
        java_env je;
        detail::java_method_sig<Ret (Args...)> sig;
        JNINativeMethod info = {
            name.data(),
            sig.signature().data(),
            reinterpret_cast<void*>(fn)
        };
        if ( je.env()->RegisterNatives(class_, &info, 1) != 0 ) {
            throw std::runtime_error("can't register native method");
        }
    }

    template < typename Ret, typename ...Args >
    void java_class::register_method(str_view name, Ret (JNICALL *fn)(JNIEnv*, jobject, Args...)) const {
        if ( !class_ ) {
            throw std::runtime_error("invalid java class");
        }
        java_env je;
        detail::java_method_sig<Ret (Args...)> sig;
        JNINativeMethod info = {
            name.data(),
            sig.signature().data(),
            reinterpret_cast<void*>(fn)
        };
        if ( je.env()->RegisterNatives(class_, &info, 1) != 0 ) {
            throw std::runtime_error("can't register native method");
        }
    }

    //
    // java_obj
    //

    class java_obj final {
    public:
        java_obj() noexcept = default;
        java_obj(java_obj&&) noexcept;
        java_obj(const java_obj&) noexcept;
        explicit java_obj(jobject) noexcept;
        template < typename ...Args >
        explicit java_obj(const java_class&, const Args&... args);
        ~java_obj() noexcept;
        java_obj& operator = (const java_obj&) noexcept;
        java_obj& operator = (java_obj&&) noexcept;
        [[nodiscard]] jobject get() const noexcept;
        template < typename FN >
        [[nodiscard]] java_method<FN> method(str_view name) const;
        template < typename FN >
        [[nodiscard]] java_static_method<FN> static_method(str_view name) const;
        [[nodiscard]] explicit operator bool() const noexcept;
    private:
        void set_(const java_env&, jobject) noexcept;
        void dec_ref_() noexcept;
    private:
        jobject obj_ = nullptr;
    };
    
    //
    // java_call_exception
    //

    class java_call_exception final : public std::exception {
    public:
        // use java_env::exception_clear() to continue or
        // return from native code to java code to process exceptions

        const char* what() const noexcept final {
            return "exception has occured when calling java method";
        }
    };

    namespace detail
    {
        //
        // java_method_caller
        //

        template < typename Ret >
        struct java_method_caller;
    
        template <>
        struct java_method_caller<void> {
            template < typename ...Args >
            static java_method_result<void> call_static(const java_class& jc, jmethodID method, Args&&... args) {
                java_env je;
                E2D_ASSERT(! je.has_exception());
                je.env()->CallStaticVoidMethod(jc.get(), method, std::forward<Args>(args)...);
                if ( je.has_exception() ) {
                    throw java_call_exception();
                }
                return java_method_result<void>();
            }
        
            template < typename ...Args >
            static java_method_result<void> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
                java_env je;
                E2D_ASSERT(! je.has_exception());
                je.env()->CallNonvirtualVoidMethod(obj.get(), jc.get(), method, std::forward<Args>(args)...);
                if ( je.has_exception() ) {
                    throw java_call_exception();
                }
                return java_method_result<void>();
            }
        
            template < typename ...Args >
            static java_method_result<void> call(const java_obj& obj, jmethodID method, Args&&... args) {
                java_env je;
                E2D_ASSERT(! je.has_exception());
                je.env()->CallVoidMethod(obj.get(), method, std::forward<Args>(args)...);
                if ( je.has_exception() ) {
                    throw java_call_exception();
                }
                return java_method_result<void>();
            }
        };

        template <>
        struct java_method_caller<jstring> {
            template < typename ...Args >
            static java_method_result<jstring> call_static(const java_class& jc, jmethodID method, Args&&... args) {
                java_env je;
                E2D_ASSERT(! je.has_exception());
                auto result = static_cast<jstring>(java_env().env()->CallStaticObjectMethod(jc.get(), method, std::forward<Args>(args)...));
                if ( je.has_exception() ) {
                    throw java_call_exception();
                }
                return result;
            }
        
            template < typename ...Args >
            static java_method_result<jstring> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
                java_env je;
                E2D_ASSERT(! je.has_exception());
                auto result = static_cast<jstring>(java_env().env()->CallNonvirtualObjectMethod(obj.get(), jc.get(), method, std::forward<Args>(args)...));
                if ( je.has_exception() ) {
                    throw java_call_exception();
                }
                return result;
            }
        
            template < typename ...Args >
            static java_method_result<jstring> call(const java_obj& obj, jmethodID method, Args&&... args) {
                java_env je;
                E2D_ASSERT(! je.has_exception());
                auto result = static_cast<jstring>(java_env().env()->CallObjectMethod(obj.get(), method, std::forward<Args>(args)...));
                if ( je.has_exception() ) {
                    throw java_call_exception();
                }
                return result;
            }
        };
    
        #define JAVA_METHOD_CALLER(type_name, suffix) \
            template <> \
            struct java_method_caller<type_name> { \
                template < typename ...Args > \
                static java_method_result<type_name> call_static(const java_class& jc, jmethodID method, Args&&... args) { \
                    java_env je; \
                    E2D_ASSERT(! je.has_exception()); \
                    auto result = java_env().env()->CallStatic ## suffix ## Method(jc.get(), method, std::forward<Args>(args)...); \
                    if ( je.has_exception() ) { \
                        throw java_call_exception(); \
                    } \
                    return result; \
                } \
                \
                template < typename ...Args > \
                static java_method_result<type_name> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) { \
                    java_env je; \
                    E2D_ASSERT(! je.has_exception()); \
                    auto result = java_env().env()->CallNonvirtual ## suffix ## Method(obj.get(), jc.get(), method, std::forward<Args>(args)...); \
                    if ( je.has_exception() ) { \
                        throw java_call_exception(); \
                    } \
                    return result; \
                } \
                \
                template < typename ...Args > \
                static java_method_result<type_name> call(const java_obj& obj, jmethodID method, Args&&... args) { \
                    java_env je; \
                    E2D_ASSERT(! je.has_exception()); \
                    auto result = java_env().env()->Call ## suffix ## Method(obj.get(), method, std::forward<Args>(args)...); \
                    if ( je.has_exception() ) { \
                        throw java_call_exception(); \
                    } \
                    return result; \
                } \
            }
        JAVA_METHOD_CALLER(jobject, Object);
        JAVA_METHOD_CALLER(jboolean, Boolean);
        JAVA_METHOD_CALLER(jbyte, Byte);
        JAVA_METHOD_CALLER(jchar, Char);
        JAVA_METHOD_CALLER(jshort, Short);
        JAVA_METHOD_CALLER(jint, Int);
        JAVA_METHOD_CALLER(jlong, Long);
        JAVA_METHOD_CALLER(jfloat, Float);
        JAVA_METHOD_CALLER(jdouble, Double);
        #undef JAVA_METHOD_CALLER
    
        #define CPPTYPE_TO_JAVATYPE(javatype, cpptype) \
            [[nodiscard]] inline javatype cpptype_to_java(const cpptype& x) { \
                return javatype(x); \
            }
        CPPTYPE_TO_JAVATYPE(jobject, jobject);
        CPPTYPE_TO_JAVATYPE(jstring, jstring);
        CPPTYPE_TO_JAVATYPE(jboolean, jboolean);
        CPPTYPE_TO_JAVATYPE(jbyte, jbyte);
        CPPTYPE_TO_JAVATYPE(jchar, jchar);
        CPPTYPE_TO_JAVATYPE(jshort, jshort);
        CPPTYPE_TO_JAVATYPE(jint, jint);
        CPPTYPE_TO_JAVATYPE(jlong, jlong);
        CPPTYPE_TO_JAVATYPE(jfloat, jfloat);
        CPPTYPE_TO_JAVATYPE(jdouble, jdouble);
        CPPTYPE_TO_JAVATYPE(jboolean, bool);
        CPPTYPE_TO_JAVATYPE(jint, u32);
        CPPTYPE_TO_JAVATYPE(jlong, u64);
        #undef CPPTYPE_TO_JAVATYPE

        [[nodiscard]] inline jobject cpptype_to_java(const java_obj& obj) {
            return obj.get();
        }

        [[nodiscard]] inline jstring cpptype_to_java(const java_string& jstr) {
            return jstr.get();
        }

        template < typename T >
        [[nodiscard]] inline auto cpptype_to_java(const java_array<T>& arr) {
            return arr.get();
        }
    }

    //
    // java_static_method
    //
    
    template < typename Ret, typename ...Args >
    class java_static_method<Ret (Args...)> {
    public:
        using signature_t = Ret (Args...);
    public:
        java_static_method() noexcept = default;

        java_static_method(const java_class& jc, jmethodID method)
        : class_(jc)
        , method_(method) {}

        template < typename ...ArgTypes >
        detail::java_method_result<Ret> operator () (const ArgTypes&... args) const {
            return detail::java_method_caller<Ret>::call_static(class_, method_, detail::cpptype_to_java(args)...);
        }
    private:
        java_class class_;
        jmethodID method_ = nullptr;
    };

    //
    // java_method
    //

    template < typename Ret, typename ...Args >
    class java_method<Ret (Args...)> {
    public:
        using signature_t = Ret (Args...);
    public:
        java_method() noexcept = default;

        java_method(const java_obj& obj, jmethodID method)
        : obj_(obj)
        , method_(method) {}
        
        template < typename ...ArgTypes >
        detail::java_method_result<Ret> operator () (const ArgTypes&... args) const {
            return detail::java_method_caller<Ret>::call(obj_, method_, detail::cpptype_to_java(args)...);
        }
    private:
        java_obj obj_;
        jmethodID method_ = nullptr;
    };
    

    template < typename FN >
    inline java_static_method<FN> java_class::static_method(str_view name) const {
        if ( !class_ ) {
            throw std::runtime_error("invalid java class");
        }
        java_env je;
        detail::java_method_sig<FN> sig;
        return java_static_method<FN>(*this, je.env()->GetStaticMethodID(class_, name.data(), sig.signature().data()));
    }

    template < typename FN >
    inline java_static_method<FN> java_obj::static_method(str_view name) const {
        return java_class(*this).static_method<FN>(name);
    }
    
    template < typename FN >
    inline java_method<FN> java_obj::method(str_view name) const {
        if ( !obj_ ) {
            throw std::runtime_error("invalid java object");
        }
        java_env je;
        detail::java_method_sig<FN> sig;
        return java_method<FN>(*this, je.env()->GetMethodID(java_class(*this).get(), name.data(), sig.signature().data()));
    }

    template < typename ...Args >
    inline java_obj::java_obj(const java_class& jc, const Args&... args) {
        if ( !jc ) {
            throw std::runtime_error("invalid java class");
        }
        java_env je;
        detail::java_method_sig<void (Args...)> sig;
        jmethodID ctor_id = je.env()->GetMethodID(jc.get(), "<init>", sig.signature().data());
        if ( !ctor_id ) {
            throw std::runtime_error("constructor doesn't exists");
        }
        jobject jo = je.env()->NewObjectV(jc.get(), std::forward<Args>(args)...);
        if ( !jo ) {
            throw std::runtime_error("failed to create java object");
        }
        set_(je, jo);
    }
  
}

#endif
