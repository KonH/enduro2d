/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "debug.hpp"

#if defined(E2D_DEBUG_CONSOLE_SINK_MODE) && E2D_DEBUG_CONSOLE_SINK_MODE == E2D_DEBUG_CONSOLE_SINK_MODE_MSVC

#include <Windows.h>

namespace e2d
{
    bool debug_console_sink::on_message(debug::level lvl, str_view text) noexcept {
        try {
			// TODO: should be <file>(<line>): <message> <newline>
			// write to IDE console
			OutputDebugStringA(text.data());
			OutputDebugStringA("\n");

			// write to console window
            const str log_text = log_text_format(lvl, text);
            const std::ptrdiff_t rprintf = std::printf("%s", log_text.c_str());
            return rprintf >= 0
                && math::numeric_cast<std::size_t>(rprintf) == log_text.length();
        } catch (...) {
            return false;
        }
    }
}

#endif
