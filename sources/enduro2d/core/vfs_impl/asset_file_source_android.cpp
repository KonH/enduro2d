/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "vfs.hpp"

#ifdef __INTELLISENSE__
#  define E2D_ASSET_MODE E2D_ASSET_MODE_ANDROID
#endif

#if defined(E2D_ASSET_MODE) && E2D_ASSET_MODE == E2D_ASSET_MODE_ANDROID

#include <enduro2d/core/platform_impl/platform_android.hpp>
#include <android/asset_manager.h>

namespace
{
    using namespace e2d;

    //
    // android_input_stream
    //

    class android_input_stream final : public input_stream {
    public:
        android_input_stream(AAsset*);
        ~android_input_stream() noexcept;
        std::size_t read(void* dst, std::size_t size);
        std::size_t seek(std::ptrdiff_t offset, bool relative);
        std::size_t tell() const;
        std::size_t length() const noexcept;
    private:
        AAsset* asset_ = nullptr;
        std::size_t length_ = 0;
    };

    android_input_stream::android_input_stream(AAsset* asset)
    : asset_(asset)
    , length_(AAsset_getLength(asset_)) {
        if ( !asset ) {
            throw bad_stream_operation();
        }
    }

    android_input_stream::~android_input_stream() noexcept {
        if ( asset_ ) {
            AAsset_close(asset_);
        }
    }

    std::size_t android_input_stream::read(void* dst, std::size_t size) {
        return AAsset_read(asset_, dst, size);
    }
    
    std::size_t android_input_stream::seek(std::ptrdiff_t offset, bool relative) {
        return AAsset_seek(asset_, offset, relative ? SEEK_CUR : SEEK_SET);
    }

    std::size_t android_input_stream::tell() const {
        return length_ - AAsset_getRemainingLength(asset_);
    }

    std::size_t android_input_stream::length() const noexcept {
        return length_;
    }
}

namespace e2d
{
    //
    // asset_file_source::state
    //

    class asset_file_source::state final {
    public:
        state(AAssetManager* asset_mngr)
        : asset_mngr_(asset_mngr) {}

        ~state() noexcept = default;

        AAssetManager* asset_manager() const noexcept {
            return asset_mngr_;
        }

    private:
        AAssetManager* asset_mngr_;
    };
    
    //
    // asset_file_source
    //

    asset_file_source::asset_file_source()
    : state_(new state(static_cast<const platform_internal_state_android&>(the<platform>().state()).asset_manager())) {
    }

    asset_file_source::~asset_file_source() noexcept = default;

    bool asset_file_source::valid() const noexcept {
        return state_->asset_manager() != nullptr;
    }

    bool asset_file_source::exists(str_view path) const {
        AAsset* asset = AAssetManager_open(state_->asset_manager(), path.data(), AASSET_MODE_UNKNOWN);
        if ( asset ) {
            AAsset_close(asset);
            return true;
        }
        return false;
    }

    input_stream_uptr asset_file_source::read(str_view path) const {
        AAsset* asset = AAssetManager_open(state_->asset_manager(), path.data(), AASSET_MODE_UNKNOWN);
        return input_stream_uptr(new android_input_stream(asset));
    }

    output_stream_uptr asset_file_source::write(str_view path, bool append) const {
        throw std::runtime_error("can't write to assets");
        return nullptr;
    }

    bool asset_file_source::trace(str_view path, filesystem::trace_func func) const {
        /*if ( !func ) {
            return false;
        }
        str parent_path = "";

        const auto for_each_file_in_dir = [this] () {
            AAssetDir* dir = AAssetManager_openDir(state_->asset_manager(), path.data());
            for (;;) {
                const char* asset_name = AAssetDir_getNextFileName(dir);
                if ( !asset_name ) {
                    break;
                }
                const str filename = path::combine(parent_path, asset_name);
                func(filename, false);
            }
        };*/
        return false;
    }
}

#endif
