/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/utils/java.hpp>

#if 1 //def E2D_JNI

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
    
    java_env::java_env() noexcept
    : jni_env_(nullptr)
    , must_be_detached_(false) {
        attach();
    }

    java_env::java_env(JNIEnv *env) noexcept
    : jni_env_(env)
    , must_be_detached_(false) {
        if ( !env ) {
            attach();
        }
    }

    java_env::~java_env() noexcept {
        detach();
    }

    void java_env::attach() {
        JavaVM* jvm = detail::java_vm::get();
        if ( !jvm ) {
            throw std::runtime_error("JavaVM is null");
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
                throw std::runtime_error("can't attach to current java thread");
            }
            must_be_detached_ = true;
            return;
        }
        throw std::runtime_error("can't get java enviroment");
    }

    void java_env::detach() {
        if ( must_be_detached_ ) {
            JavaVM* jvm = detail::java_vm::get();
            if ( !jvm ) {
                throw std::runtime_error("JavaVM is null");
            }
            jvm->DetachCurrentThread();
            jni_env_ = nullptr;
            must_be_detached_ = false;
        }
    }

    void java_env::inc_global_ref(jobject obj) const {
        E2D_ASSERT(jni_env_);
        jni_env_->NewGlobalRef(obj);
    }

    void java_env::dec_global_ref(jobject obj) const {
        E2D_ASSERT(jni_env_);
        jni_env_->DeleteGlobalRef(obj);
    }

    void java_env::inc_local_ref(jobject obj) const {
        E2D_ASSERT(jni_env_);
        jni_env_->NewLocalRef(obj);
    }

    void java_env::dec_local_ref(jobject obj) const {
        E2D_ASSERT(jni_env_);
        jni_env_->DeleteLocalRef(obj);
    }
    
    bool java_env::has_exception() const noexcept {
        E2D_ASSERT(jni_env_);
        return jni_env_->ExceptionCheck();
    }

    JNIEnv* java_env::env() const noexcept {
        E2D_ASSERT(jni_env_);
        return jni_env_;
    }

    void java_env::inc_ref(jobject obj, bool global) const {
        E2D_ASSERT(jni_env_);
        E2D_ASSERT(obj);
        if ( global ) {
            jni_env_->NewGlobalRef(obj);
        } else {
            jni_env_->NewLocalRef(obj);
        }
    }

    void java_env::dec_ref(jobject obj, bool global) const {
        E2D_ASSERT(jni_env_);
        E2D_ASSERT(obj);
        if ( global ) {
            jni_env_->DeleteGlobalRef(obj);
        } else {
            jni_env_->DeleteLocalRef(obj);
        }
    }

    //
    // java_class
    //

    java_class::java_class(str_view class_name) {
        java_env je;
        class_ = je.env()->FindClass(class_name.data());
        if ( !class_ ) {
            throw std::runtime_error("java class is not found");
        }
        global_ = (je.env()->GetObjectRefType(class_) == JNIGlobalRefType);
        inc_ref_();
    }

    java_class::java_class(jclass jc, bool global_ref) noexcept
    : class_(jc)
    , global_(global_ref) {
        inc_ref_();
    }
    
    java_class::java_class(const java_obj& obj) {
        if ( !obj ) {
            throw std::runtime_error("java object is null");
        }
        java_env je;
        class_ = je.env()->GetObjectClass(obj.data());
        if ( !class_ ) {
            throw std::runtime_error("failed to get object class");
        }
        global_ = (je.env()->GetObjectRefType(class_) == JNIGlobalRefType);
        inc_ref_();
    }

    java_class::java_class(java_class&& jc) noexcept
    : class_(jc.class_)
    , global_(jc.global_) {
        jc.class_ = nullptr;
    }
    
    java_class::java_class(const java_class& jc) noexcept
    : class_(jc.class_)
    , global_(jc.global_) {
        inc_ref_();
    }

    java_class::~java_class() noexcept {
        dec_ref_();
    }
    
    java_class& java_class::operator = (const java_class& jc) noexcept {
        dec_ref_();
        class_ = jc.data();
        global_ = jc.global_;
        inc_ref_();
        return *this;
    }

    java_class& java_class::operator = (java_class&& jc) noexcept {
        dec_ref_();
        global_ = jc.global_;
        class_ = jc.class_;
        jc.class_ = nullptr;
        return *this;
    }

    java_class::operator bool() const noexcept {
        return class_ != nullptr;
    }

    void java_class::inc_ref_() noexcept {
        if ( class_ ) {
            java_env().inc_ref(static_cast<jobject>(class_), global_);
        }
    }

    void java_class::dec_ref_() noexcept {
        if ( class_ ) {
            java_env().dec_ref(static_cast<jobject>(class_), global_);
            class_ = nullptr;
        }
    }
    
    jclass java_class::data() const noexcept {
        return class_;
    }
    
    //
    // java_obj
    //
    
    java_obj::java_obj() noexcept
    : obj_(nullptr) {
    }
    
    java_obj::java_obj(const java_obj& obj) noexcept
    : obj_(obj.data()) {
        inc_ref_();
    }

    java_obj::java_obj(java_obj&& obj) noexcept
    : obj_(obj.data()) {
        obj.obj_ = nullptr;
    }

    java_obj::java_obj(jobject obj) noexcept
    : obj_(obj) {
        inc_ref_();
    }

    java_obj::~java_obj() noexcept {
        dec_ref_();
    }
    
    java_obj& java_obj::operator = (const java_obj& obj) noexcept {
        dec_ref_();
        obj_ = obj.data();
        inc_ref_();
        return *this;
    }

    java_obj& java_obj::operator = (java_obj&& obj) noexcept {
        std::swap(obj_, obj.obj_);
        return *this;
    }

    void java_obj::inc_ref_() noexcept {
        if ( obj_ ) {
            java_env().inc_global_ref(obj_);
        }
    }

    void java_obj::dec_ref_() noexcept {
        if ( obj_ ) {
            java_env().dec_global_ref(obj_);
            obj_ = nullptr;
        }
    }
    
    jobject java_obj::data() const noexcept {
        return obj_;
    }

    java_obj::operator bool() const noexcept {
        return obj_ != nullptr;
    }
 
}

#endif
