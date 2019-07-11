/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform_impl/platform.hpp"

namespace e2d
{
    //
    // platform::internal_state
    //

    platform::internal_state::internal_state(int argc, char *argv[]) {
        if ( argc > 0 ) {
            command_line_arguments_.reserve(
                math::numeric_cast<std::size_t>(argc));
            for ( int i = 0; i < argc; ++i ) {
                command_line_arguments_.emplace_back(argv[i]);
            }
        }
    }

    const vector<str>& platform::internal_state::command_line_arguments() const noexcept {
        return command_line_arguments_;
    }

    //
    // platform
    //

    platform::~platform() noexcept = default;

    std::size_t platform::command_line_argument_count() const noexcept {
        return state_->command_line_arguments().size();
    }

    const str& platform::command_line_argument(std::size_t index) const noexcept {
        E2D_ASSERT(index < state_->command_line_arguments().size());
        return state_->command_line_arguments()[index];
    }
        
    const platform::internal_state& platform::state() const noexcept {
        return *state_;
    }

    void platform::override_predef_paths(vfs& the_vfs) {
        return state_->override_predef_paths(the_vfs);
    }
}
