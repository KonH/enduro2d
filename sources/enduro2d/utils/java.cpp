/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "java.hpp"

#ifdef E2D_JNI

namespace e2d
{
    //
    // java_vm
    //
    
    JavaVM* detail::java_vm::get() noexcept {
        return get_jvm_();
    }
    
    void detail::java_vm::set(JavaVM* vm) noexcept {
        get_jvm_() = vm;
    }

    JavaVM*& detail::java_vm::get_jvm_() noexcept {
        static JavaVM* instance;
        return instance;
    }
    
    //
    // java_env
    //
    
    java_env::java_env() noexcept {
        attach();
    }

    java_env::java_env(JNIEnv *env) noexcept
    : jni_env_(env) {
        attach();
    }

    java_env::~java_env() noexcept {
        detach();
    }

    void java_env::attach() {
        if ( jni_env_ ) {
            return;
        }
        JavaVM* jvm = detail::java_vm::get();
        if ( !jvm ) {
            throw java_exception("JavaVM is null");
        }
        detach();
        void* env = nullptr;
        jint status = jvm->GetEnv(&env, JNI_VERSION_1_6);
        jni_env_ = static_cast<JNIEnv*>(env);
        if ( status == JNI_OK ) {
            must_be_detached_ = false;
            return;
        }
        if ( status == JNI_EDETACHED ) {
            if ( jvm->AttachCurrentThread(&jni_env_, nullptr) < 0 ) {
                throw java_exception("can't attach to current java thread");
            }
            must_be_detached_ = true;
            return;
        }
        throw java_exception("can't get java enviroment");
    }

    void java_env::detach() {
        if ( must_be_detached_ ) {
            JavaVM* jvm = detail::java_vm::get();
            if ( !jvm ) {
                throw java_exception("JavaVM is null");
            }
            jvm->DetachCurrentThread();
            jni_env_ = nullptr;
            must_be_detached_ = false;
        }
    }
    
    void java_env::throw_exception(str_view msg) const {
        E2D_ASSERT(jni_env_);
        java_class jc("java/lang/Error");
        jni_env_->ThrowNew(jc.get(), msg.data());
    }

    void java_env::exception_clear() const noexcept {
        E2D_ASSERT(jni_env_);
        jni_env_->ExceptionClear();
    }

    bool java_env::has_exception() const noexcept {
        E2D_ASSERT(jni_env_);
        return jni_env_->ExceptionCheck() == JNI_TRUE;
    }

    JNIEnv* java_env::env() const noexcept {
        E2D_ASSERT(jni_env_);
        return jni_env_;
    }
    
    //
    // java_string
    //

    java_string::java_string(jstring jstr) noexcept {
        java_env je;
        jstr_ = jstr;
        data_ = je.env()->GetStringUTFChars(jstr, 0);
        length_ = je.env()->GetStringUTFLength(jstr);
    }

    java_string::java_string(str_view strv) noexcept
    : from_native_code_(true) {
        java_env je;
        jstr_ = je.env()->NewStringUTF(strv.data());
    }

    java_string::java_string(java_string&& other) noexcept
    : from_native_code_(other.from_native_code_) {
        std::swap(data_, other.data_);
        std::swap(jstr_, other.jstr_);
        std::swap(length_, other.length_);
    }
    
    java_string::~java_string() noexcept {
        release_();
    }
    
    void java_string::release_() noexcept {
        java_env je;
        if ( from_native_code_ ) {
            if ( jstr_ ) {
                je.env()->DeleteLocalRef(jstr_);
            }
        } else if ( data_ && jstr_ ) {
            je.env()->ReleaseStringUTFChars(jstr_, data_);
        }
        data_ = nullptr;
        jstr_ = nullptr;
        length_ = 0;
    }
    
    const char* java_string::data() const noexcept {
        return data_;
    }
    
    size_t java_string::length() const noexcept {
        return length_;
    }
    
    jstring java_string::get() const noexcept {
        return jstr_;
    }
    
    java_string::operator str_view() const noexcept {
        E2D_ASSERT(data_);
        return str_view(data_, length_);
    }

    //
    // java_class
    //

    java_class::java_class(str_view class_name) {
        java_env je;
        jclass jc = je.env()->FindClass(class_name.data());
        if ( !jc ) {
            throw java_exception("java class is not found");
        }
        set_(je, jc);
    }

    java_class::java_class(jclass jc) noexcept {
        set_(java_env(), jc);
    }
    
    java_class::java_class(const java_obj& obj) {
        if ( !obj ) {
            throw java_exception("java object is null");
        }
        java_env je;
        jclass jc = je.env()->GetObjectClass(obj.get());
        if ( !jc ) {
            throw java_exception("failed to get object class");
        }
        set_(je, jc);
    }

    java_class::java_class(java_class&& jc) noexcept
    : class_(jc.class_) {
        jc.class_ = nullptr;
    }
    
    java_class::java_class(const java_class& jc) noexcept {
        set_(java_env(), jc.get());
    }

    java_class::~java_class() noexcept {
        dec_ref_();
    }

    java_class& java_class::operator = (const java_class& jc) noexcept {
        dec_ref_();
        set_(java_env(), jc.get());
        return *this;
    }

    java_class& java_class::operator = (java_class&& jc) noexcept {
        dec_ref_();
        class_ = jc.class_;
        jc.class_ = nullptr;
        return *this;
    }

    java_class::operator bool() const noexcept {
        return class_ != nullptr;
    }
    
    void java_class::set_(const java_env& je, jclass jc) noexcept {
        if ( jc ) {
            class_ = static_cast<jclass>(je.env()->NewGlobalRef(jc));
        }
    }

    void java_class::dec_ref_() noexcept {
        if ( class_ ) {
            java_env().env()->DeleteGlobalRef(class_);
            class_ = nullptr;
        }
    }
    
    jclass java_class::get() const noexcept {
        return class_;
    }
    
    //
    // java_obj
    //
    
    java_obj::java_obj(const java_obj& jo) noexcept {
        set_(java_env(), jo.get());
    }

    java_obj::java_obj(java_obj&& jo) noexcept
    : obj_(jo.get()) {
        jo.obj_ = nullptr;
    }

    java_obj::java_obj(jobject jo) noexcept {
        set_(java_env(), jo);
    }

    java_obj::~java_obj() noexcept {
        dec_ref_();
    }
    
    java_obj& java_obj::operator = (const java_obj& jo) noexcept {
        dec_ref_();
        set_(java_env(), jo.get());
        return *this;
    }

    java_obj& java_obj::operator = (java_obj&& jo) noexcept {
        dec_ref_();
        obj_ = jo.get();
        jo.obj_ = nullptr;
        return *this;
    }
    
    void java_obj::set_(const java_env& je, jobject jo) noexcept {
        if ( jo ) {
            obj_ = je.env()->NewGlobalRef(jo);
        }
    }

    void java_obj::dec_ref_() noexcept {
        if ( obj_ ) {
            java_env().env()->DeleteGlobalRef(obj_);
            obj_ = nullptr;
        }
    }
    
    jobject java_obj::get() const noexcept {
        return obj_;
    }

    java_obj::operator bool() const noexcept {
        return obj_ != nullptr;
    }
 
}

#endif
