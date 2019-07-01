/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "_utils.hpp"

#if defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_ANDROID
#  define E2D_JNI
#endif

#ifdef E2D_JNI

#include <jni.h>

namespace e2d
{
    //
    // java_vm
    //

    class java_vm {
    public:
        [[nodiscard]] static JavaVM* get() noexcept;
    private:
        static JavaVM*& get_jvm() noexcept;
    };

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
        void inc_global_ref(jobject) const;
        void dec_global_ref(jobject) const;
        void inc_local_ref(jobject) const;
        void dec_local_ref(jobject) const;
        [[nodiscard]] bool has_exception() const noexcept;
        [[nodiscard]] JNIEnv* env() const noexcept;
    private:
        JNIEnv* jni_env_;
        bool must_be_detached_;
    };
    
    template < typename T >
    struct jni_type_name;
    
    template < typename T >
    struct jni_type_name<T*> {
        using type = T*;
        void append(str& s) { s += '[';  jni_type_name<T>::append(s); }
    };

    template <>
    struct jni_type_name<void> {
        using type = void;
        void append(str& s) { s += 'V'; }
    };
    
    template <>
    struct jni_type_name<jboolean> {
        using type = jboolean;
        void append(str& s) { s += 'Z'; }
    };
    
    template <>
    struct jni_type_name<jbyte> {
        using type = jbyte;
        void append(str& s) { s += 'B'; }
    };
    
    template <>
    struct jni_type_name<jchar> {
        using type = jchar;
        void append(str& s) { s += 'C'; }
    };
    
    template <>
    struct jni_type_name<jshort> {
        using type = jshort;
        void append(str& s) { s += 'S'; }
    };
    
    template <>
    struct jni_type_name<jint> {
        using type = jint;
        void append(str& s) { s += 'I'; }
    };
    
    template <>
    struct jni_type_name<jlong> {
        using type = jlong;
        void append(str& s) { s += 'J'; }
    };
    
    template <>
    struct jni_type_name<jfloat> {
        using type = jfloat;
        void append(str& s) { s += 'F'; }
    };
    
    template <>
    struct jni_type_name<jdouble> {
        using type = jdouble;
        void append(str& s) { s += 'D'; }
    };
    
    template <>
    struct jni_type_name<jstring> {
        using type = jstring;
        void append(str& s) { s += "Ljava/lang/String;"; }
    };
    
    template <>
    struct jni_type_name<jobject> {
        using type = jobject;
        void append(str& s) { s += "Ljava/lang/Object;"; }
    };
    
    template <>
    struct jni_type_name<jthrowable> {
        using type = jthrowable;
        void append(str& s) { s += "Ljava/lang/Throwable;"; }
    };
    
    template <>
    struct jni_type_name<jbyteArray> {
        using type = jbyteArray;
        void append(str& s) { s += "[B"; }
    };
    
    template <>
    struct jni_type_name<jcharArray> {
        using type = jcharArray;
        void append(str& s) { s += "[C"; }
    };
    
    template <>
    struct jni_type_name<jshortArray> {
        using type = jshortArray;
        void append(str& s) { s += "[S"; }
    };
    
    template <>
    struct jni_type_name<jintArray> {
        using type = jintArray;
        void append(str& s) { s += "[I"; }
    };
    
    template <>
    struct jni_type_name<jlongArray> {
        using type = jlongArray;
        void append(str& s) { s += "[J"; }
    };
    
    template <>
    struct jni_type_name<jfloatArray> {
        using type = jfloatArray;
        void append(str& s) { s += "[F"; }
    };
    
    template <>
    struct jni_type_name<jdoubleArray> {
        using type = jdoubleArray;
        void append(str& s) { s += "[D"; }
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
    
    template <>
    struct java_method_result<void> {
    };
    
    template < typename T >
    inline java_method_result<T>::java_method_result(T value) noexcept
    : value_(value) {
    }

    template < typename T >
    inline java_method_result<T>::operator T() const noexcept {
        return value_;
    }

    //
    // java_method_sig
    //

    class java_method_sig final {
    public:
        java_method_sig() noexcept = default;
        java_method_sig(str_view);
        template < typename Ret, typename ...Args >
        java_method_sig(stdex::in_place_type_t<Ret (Args...)>);
        [[nodiscard]] const str& signature() const noexcept;
    private:
        template < typename Arg0, typename ...Args >
        void append_args();
    private:
        str signature_;
    };
    
    template < typename Ret, typename ...Args >
    inline java_method_sig::java_method_sig(stdex::in_place_type_t<Ret (Args...)>) {
        signature_ += '(';
        append_args<Args...>();
        signature_ += ')';
        jni_type_name<Ret>::append(signature_);
    }
    
    template < typename Arg0, typename ...Args >
    inline void java_method_sig::append_args() {
        jni_type_name<Arg0>::append(signature_);
        append_args<Args...>();
    }

    //
    // java_class
    //

    class java_class final {
    public:
        explicit java_class(str_view);
        explicit java_class(jclass cl) noexcept;
        explicit java_class(const class java_obj&);
        template < typename T >
        explicit java_class(stdex::in_place_type_t<T>);
        java_class(java_class&&) noexcept;
        java_class(const java_class&) noexcept;
        ~java_class() noexcept;
        [[nodiscard]] jclass data() const noexcept;
        [[nodiscard]] jmethodID method_id(str_view name, const java_method_sig& sig) const;
        [[nodiscard]] jmethodID static_method_id(str_view name, const java_method_sig& sig) const;
        [[nodiscard]] explicit operator bool() const noexcept;
    private:
        void inc_ref() noexcept;
        void dec_ref() noexcept;
    private:
        jclass class_;
    };
    
    template < typename T >
    inline java_class::java_class(stdex::in_place_type_t<T>) {
        java_env je;
        str class_name;
        jni_type_name<T>::append(class_name);
        // remove first 'L'
        if ( class_name.length() > 0 and class_name.front() == 'L' ) {
            class_name = class_name.substr(1);
        }
        // remove last ';'
        if ( class_name.length() and class_name.back() == ';' ) {
            class_name.erase(class_name.end()-1);
        }
        class_ = je.env()->FindClass(class_name.data());
        if ( !class_ ) {
            throw std::runtime_error("java class is not found");
        }
        inc_ref();
    }

    //
    // java_obj
    //

    class java_obj final {
    public:
        java_obj() noexcept;
        java_obj(java_obj&&) noexcept;
        java_obj(const java_obj&) noexcept;
        explicit java_obj(jobject) noexcept;
        template < typename ...Args >
        explicit java_obj(const java_class&, const Args&... args);
        ~java_obj() noexcept;
        [[nodiscard]] jobject data() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept;
    private:
        void inc_ref() noexcept;
        void dec_ref() noexcept;
    private:
        jobject obj_;
    };
    
    template < typename ...Args >
    inline java_obj::java_obj(const java_class& jc, const Args&... args) {
        java_env je;
        java_method_sig sig(stdex::in_place_type<void (Args...)>);
        jmethodID ctor_id = jc.method_id("<init>", sig);
        if ( !ctor_id ) {
            throw std::runtime_error("constructor doesn't exists");
        }
        obj_ = je.env()->NewObjectV(jc.data(), std::forward<Args>(args)...);
        if ( !obj_ ) {
            throw std::runtime_error("failed to create java object");
        }
        inc_ref();
    }
    

    //
    // java_method_caller
    //

    template < typename Ret >
    struct java_method_caller;
    
    template <>
    struct java_method_caller<void> {
        template < typename ...Args >
        static java_method_result<void> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            java_env().env()->CallStaticVoidMethod(jc.data(), method, std::forward<Args>(args)...);
            return java_method_result<void>();
        }
        
        template < typename ...Args >
        static java_method_result<void> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            java_env().env()->CallNonvirtualVoidMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
            return java_method_result<void>();
        }
        
        template < typename ...Args >
        static java_method_result<void> call(const java_obj& obj, jmethodID method, Args&&... args) {
            java_env().env()->CallVoidMethod(obj.data(), method, std::forward<Args>(args)...);
            return java_method_result<void>();
        }
    };
    
    template <>
    struct java_method_caller<jobject> {
        template < typename ...Args >
        static java_method_result<jobject> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticObjectMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jobject> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualObjectMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jobject> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallObjectMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct java_method_caller<jboolean> {
        template < typename ...Args >
        static java_method_result<jboolean> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticBooleanMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jboolean> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualBooleanMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jboolean> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallBooleanMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct java_method_caller<jbyte> {
        template < typename ...Args >
        static java_method_result<jbyte> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticByteMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jbyte> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualByteMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jbyte> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallByteMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct java_method_caller<jchar> {
        template < typename ...Args >
        static java_method_result<jchar> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticShortMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jchar> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualShortMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jchar> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallShortMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct java_method_caller<jint> {
        template < typename ...Args >
        static java_method_result<jint> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticIntMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jint> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualIntMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jint> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallIntMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct java_method_caller<jlong> {
        template < typename ...Args >
        static java_method_result<jlong> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticLongMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jlong> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualLongMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jlong> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallLongMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct java_method_caller<jfloat> {
        template < typename ...Args >
        static java_method_result<jfloat> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticFloatMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jfloat> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualFloatMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jfloat> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallFloatMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };
    
    template <>
    struct java_method_caller<jdouble> {
        template < typename ...Args >
        static java_method_result<jdouble> call_static(const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallStaticDoubleMethod(jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jdouble> call_nonvirtual(const java_obj& obj, const java_class& jc, jmethodID method, Args&&... args) {
            return java_env().env()->CallNonvirtualDoubleMethod(obj.data(), jc.data(), method, std::forward<Args>(args)...);
        }
        
        template < typename ...Args >
        static java_method_result<jdouble> call(const java_obj& obj, jmethodID method, Args&&... args) {
            return java_env().env()->CallDoubleMethod(obj.data(), method, std::forward<Args>(args)...);
        }
    };

    //
    // java_static_method
    //

    template < typename FN >
    class java_static_method;
    
    template < typename Ret, typename ...Args >
    class java_static_method<Ret (Args...)> {
    public:
        java_static_method() noexcept = default;
        java_static_method(java_static_method&&) noexcept = default;
        java_static_method(const java_static_method&) noexcept = default;

        java_static_method(const java_class& jc, str_view name)
        : class_(jc)
        , method_(nullptr) {
            java_method_sig sig(stdex::in_place_type<void (Args...)>);
            method_ = class_.static_method_id(name, sig);
        }

        java_method_result<Ret> operator () (const Args&... args) const {
            return java_method_caller<Ret>::call_static(class_, method_, args...);
        }
    private:
        java_class class_;
        jmethodID method_ {nullptr};
    };

    //
    // java_method
    //
    
    template < typename FN >
    class java_method;

    template < typename Ret, typename ...Args >
    class java_method<Ret (Args...)> {
    public:
        java_method() noexcept = default;
        java_method(java_method&&) noexcept = default;
        java_method(const java_method&) noexcept = default;

        java_method(const java_obj& obj, str_view name)
        : obj_(obj)
        , method_(nullptr) {
            java_method_sig sig(stdex::in_place_type<void (Args...)>);
            method_ = java_class(obj_).method_id(name, sig);
        }

        java_method_result<Ret> operator () (const Args&... args) const {
            return java_method_caller<Ret>::call(obj_, method_, args...);
        }
    private:
        java_obj obj_;
        jmethodID method_ {nullptr};
    };

}

#endif
