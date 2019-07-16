/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#define CATCH_CONFIG_RUNNER
#include "_base.hpp"

TEST_CASE("base") {
}

int e2d_main (int argc, char * argv[]) {
    // without platform android can not start tests
    using namespace e2d;
    if ( !modules::is_initialized<platform>() ) {
        modules::initialize<platform>(argc, argv);
    }

    return Catch::Session().run( argc, argv );
}
