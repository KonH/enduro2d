/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "common.hpp"
    
#if E2D_PLATFORM == E2D_PLATFORM_ANDROID
#include <android/log.h>
#endif

namespace e2d_untests
{
#if E2D_PLATFORM == E2D_PLATFORM_ANDROID
    void catch2_test_listener::assertionStarting(const Catch::AssertionInfo& info) {
    }

    bool catch2_test_listener::assertionEnded(const Catch::AssertionStats& info) {
        if ( !info.assertionResult.isOk() ) {
            __android_log_print(ANDROID_LOG_ERROR, "enduro2d.untests",
                "%s (%zd): %s\n",
                info.assertionResult.m_info.lineInfo.file,
                info.assertionResult.m_info.lineInfo.line,
                info.assertionResult.m_info.capturedExpression.c_str());
        }
        return true;
    }
    
    CATCH_REGISTER_LISTENER(catch2_test_listener)
#endif
}