/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "_core.hpp"

namespace e2d
{
    class platform final : public module<platform> {
    public:
        class internal_state;
        using internal_state_uptr = std::unique_ptr<internal_state>;
        const internal_state& state() const noexcept;
    public:
        platform(int argc, char *argv[]);
        ~platform() noexcept final;

        std::size_t command_line_argument_count() const noexcept;
        const str& command_line_argument(std::size_t index) const noexcept;

        void override_predef_paths(vfs&);
    private:
        internal_state_uptr state_;
    };
}
