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
    
    JavaVM* java_vm::get() noexcept {
        return get_jvm();
    }

    JavaVM*& java_vm::get_jvm() noexcept {
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
        JavaVM* jvm = java_vm::get();
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
            JavaVM* jvm = java_vm::get();
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

    //
    // java_method_sig
    //

    java_method_sig::java_method_sig(str_view sig)
    : signature_(sig) {
    }
    
    const str& java_method_sig::signature() const noexcept {
        return signature_;
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
        inc_ref();
    }

    java_class::java_class(jclass jc) noexcept
    : class_(jc) {
        inc_ref();
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
        inc_ref();
    }

    java_class::java_class(java_class&& jc) noexcept
    : class_(jc.class_) {
        jc.class_ = nullptr;
    }
    
    java_class::java_class(const java_class& jc) noexcept
    : class_(jc.class_) {
        inc_ref();
    }

    java_class::~java_class() noexcept {
        dec_ref();
    }
    
    java_class::operator bool() const noexcept {
        return class_ != nullptr;
    }

    void java_class::inc_ref() noexcept {
        if ( class_ ) {
            java_env().inc_global_ref(static_cast<jobject>(class_));
        }
    }

    void java_class::dec_ref() noexcept {
        if ( class_ ) {
            java_env().dec_global_ref(static_cast<jobject>(class_));
            class_ = nullptr;
        }
    }
    
    jclass java_class::data() const noexcept {
        return class_;
    }

    jmethodID java_class::method_id(str_view name, const java_method_sig& sig) const {
        if ( !class_ ) {
            throw std::runtime_error("invalid java class");
        }
        java_env je;
        return je.env()->GetMethodID(class_, name.data(), sig.signature().data());
    }

    jmethodID java_class::static_method_id(str_view name, const java_method_sig& sig) const {
        if ( !class_ ) {
            throw std::runtime_error("invalid java class");
        }
        java_env je;
        return je.env()->GetStaticMethodID(class_, name.data(), sig.signature().data());
    }
    
    //
    // java_obj
    //
    
    java_obj::java_obj() noexcept
    : obj_(nullptr) {
    }
    
    java_obj::java_obj(const java_obj& obj) noexcept
    : obj_(obj.obj_) {
        inc_ref();
    }

    java_obj::java_obj(java_obj&& obj) noexcept
    : obj_(obj.obj_) {
        obj.obj_ = nullptr;
    }

    java_obj::java_obj(jobject obj) noexcept
    : obj_(obj) {
        inc_ref();
    }

    java_obj::~java_obj() noexcept {
    }

    void java_obj::inc_ref() noexcept {
        if ( obj_ ) {
            java_env().inc_global_ref(obj_);
        }
    }

    void java_obj::dec_ref() noexcept {
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
