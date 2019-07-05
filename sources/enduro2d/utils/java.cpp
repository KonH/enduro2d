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
    
    bool java_env::has_exception() const noexcept {
        E2D_ASSERT(jni_env_);
        return jni_env_->ExceptionCheck();
    }

    JNIEnv* java_env::env() const noexcept {
        E2D_ASSERT(jni_env_);
        return jni_env_;
    }

    //
    // java_class
    //

    java_class::java_class(str_view class_name) {
        java_env je;
        jclass jc = je.env()->FindClass(class_name.data());
        if ( !jc ) {
            throw std::runtime_error("java class is not found");
        }
        set_(je, jc);
    }

    java_class::java_class(jclass jc) noexcept {
        set_(java_env(), jc);
    }
    
    java_class::java_class(const java_obj& obj) {
        if ( !obj ) {
            throw std::runtime_error("java object is null");
        }
        java_env je;
        jclass jc = je.env()->GetObjectClass(obj.data());
        if ( !jc ) {
            throw std::runtime_error("failed to get object class");
        }
        set_(je, jc);
    }

    java_class::java_class(java_class&& jc) noexcept
    : class_(jc.class_) {
        jc.class_ = nullptr;
    }
    
    java_class::java_class(const java_class& jc) noexcept {
        set_(java_env(), jc.data());
    }

    java_class::~java_class() noexcept {
        dec_ref_();
    }

    java_class& java_class::operator = (const java_class& jc) noexcept {
        dec_ref_();
        set_(java_env(), jc.data());
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
    
    jclass java_class::data() const noexcept {
        return class_;
    }
    
    //
    // java_obj
    //
    
    java_obj::java_obj(const java_obj& jo) noexcept {
        set_(java_env(), jo.data());
    }

    java_obj::java_obj(java_obj&& jo) noexcept
    : obj_(jo.data()) {
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
        set_(java_env(), jo.data());
        return *this;
    }

    java_obj& java_obj::operator = (java_obj&& jo) noexcept {
        dec_ref_();
        obj_ = jo.data();
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
    
    jobject java_obj::data() const noexcept {
        return obj_;
    }

    java_obj::operator bool() const noexcept {
        return obj_ != nullptr;
    }
 
}

#endif
