/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "filesystem.hpp"

#if defined(E2D_PLATFORM) && (E2D_PLATFORM == E2D_PLATFORM_LINUX || E2D_PLATFORM == E2D_PLATFORM_ANDROID)

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
    const mode_t default_directory_mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
}

namespace e2d::filesystem::impl
{
    bool remove_file(str_view path) {
        return 0 == ::unlink(make_utf8(path).c_str())
            || errno == ENOENT;
    }

    bool remove_directory(str_view path) {
        return 0 == ::rmdir(make_utf8(path).c_str())
            || errno == ENOENT;
    }

    bool file_exists(str_view path) {
        struct stat st{};
        return 0 == ::stat(make_utf8(path).c_str(), &st)
            && S_ISREG(st.st_mode);
    }

    bool directory_exists(str_view path) {
        struct stat st{};
        return 0 == ::stat(make_utf8(path).c_str(), &st)
            && S_ISDIR(st.st_mode);
    }

    bool create_directory(str_view path) {
        return 0 == ::mkdir(make_utf8(path).c_str(), default_directory_mode)
            || errno == EEXIST;
    }

    bool trace_directory(str_view path, const trace_func& func) {
        std::unique_ptr<DIR, decltype(&::closedir)> dir{
            ::opendir(make_utf8(path).c_str()),
            ::closedir};
        if ( !dir ) {
            return false;
        }
        while ( dirent* ent = ::readdir(dir.get()) ) {
            if ( 0 != std::strcmp(ent->d_name, ".") && 0 != std::strcmp(ent->d_name, "..") ) {
                if ( !func || !func(ent->d_name, DT_DIR == ent->d_type) ) {
                    return false;
                }
            }
        }
        return true;
    }
}

#endif
