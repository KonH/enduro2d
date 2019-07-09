/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "vfs.hpp"

#if defined(E2D_ASSET_MODE) && E2D_ASSET_MODE == E2D_ASSET_MODE_NONE

namespace e2d
{
    //
    // asset_file_source::state
    //

    class asset_file_source::state final {
    public:
        state();
        ~state() noexcept;
        bool valid() const noexcept;
        bool exists(str_view path) const;
        input_stream_uptr read(str_view path) const;
        output_stream_uptr write(str_view path, bool append) const;
        bool trace(str_view path, filesystem::trace_func func) const;
    private:
    };
    
    //
    // asset_file_source
    //

    asset_file_source::asset_file_source() = default;

    asset_file_source::~asset_file_source() noexcept = default;

    bool asset_file_source::valid() const noexcept {
        return false;
    }

    bool asset_file_source::exists(str_view path) const {
        E2D_UNUSED(path);
        return false;
    }

    input_stream_uptr asset_file_source::read(str_view path) const {
        E2D_UNUSED(path);
        return nullptr;
    }

    output_stream_uptr asset_file_source::write(str_view path, bool append) const {
        E2D_UNUSED(path, append);
        return nullptr;
    }

    bool asset_file_source::trace(str_view path, filesystem::trace_func func) const {
        E2D_UNUSED(path, func);
        return false;
    }
}

#endif
