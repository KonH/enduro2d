/*******************************************************************************
 * This file is part of the "enduro2d"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "filesystem.hpp"

#if defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_WINDOWS

#include <shlobj.h>
#include <windows.h>

namespace
{
    using namespace e2d;

    void safe_close_find_handle(HANDLE handle) noexcept {
        if ( handle != INVALID_HANDLE_VALUE ) {
            ::FindClose(handle);
        }
    }
}

namespace e2d::filesystem::impl
{
    bool remove_file(str_view path) {
        const wstr wide_path = make_wide(path);
        return ::DeleteFileW(wide_path.c_str())
            || ::GetLastError() == ERROR_FILE_NOT_FOUND
            || ::GetLastError() == ERROR_PATH_NOT_FOUND;
    }

    bool remove_directory(str_view path) {
        const wstr wide_path = make_wide(path);
        return ::RemoveDirectoryW(wide_path.c_str())
            || ::GetLastError() == ERROR_FILE_NOT_FOUND
            || ::GetLastError() == ERROR_PATH_NOT_FOUND;
    }

    bool file_exists(str_view path) {
        const wstr wide_path = make_wide(path);
        DWORD attributes = ::GetFileAttributesW(wide_path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES
            && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool directory_exists(str_view path) {
        const wstr wide_path = make_wide(path);
        DWORD attributes = ::GetFileAttributesW(wide_path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES
            && (attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool create_directory(str_view path) {
        const wstr wide_path = make_wide(path);
        return ::CreateDirectoryW(wide_path.c_str(), nullptr)
            || ::GetLastError() == ERROR_ALREADY_EXISTS;
    }

    bool trace_directory(str_view path, const trace_func& func) {
        WIN32_FIND_DATAW entw;
        std::unique_ptr<void, decltype(&safe_close_find_handle)> dir{
            ::FindFirstFileW(make_wide(path::combine(path, "*")).c_str(), &entw),
            safe_close_find_handle};
        if ( INVALID_HANDLE_VALUE == dir.get() ) {
            return false;
        }
        do {
            if ( 0 != std::wcscmp(entw.cFileName, L".") && 0 != std::wcscmp(entw.cFileName, L"..") ) {
                const str relative = make_utf8(entw.cFileName);
                const bool directory = !!(entw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                if ( !func || !func(relative, directory) ) {
                    return false;
                }
            }
        } while ( ::FindNextFileW(dir.get(), &entw) != 0 );
        return true;
    }
}

#endif
