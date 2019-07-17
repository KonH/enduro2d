/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_WINDOWS

#include <shlobj.h>
#include <windows.h>

namespace
{
    using namespace e2d;

    class platform_internal_state_windows final : public platform::internal_state {
    public:
        platform_internal_state_windows(int argc, char *argv[]);
        ~platform_internal_state_windows() noexcept;
        
        void register_scheme_aliases(vfs&) override;
    private:
        UINT timers_resolution_{0u};
    };
    
    platform_internal_state_windows::platform_internal_state_windows(int argc, char *argv[])
    : internal_state(argc, argv) {
        TIMECAPS tc;
        if ( MMSYSERR_NOERROR == ::timeGetDevCaps(&tc, sizeof(tc)) ) {
            if ( TIMERR_NOERROR == ::timeBeginPeriod(tc.wPeriodMin) ) {
                timers_resolution_ = tc.wPeriodMin;
            }
        }
    }

    platform_internal_state_windows::~platform_internal_state_windows() noexcept {
        if ( timers_resolution_ > 0 ) {
            ::timeEndPeriod(timers_resolution_);
        }
    }
       
    bool extract_home_directory(str& dst) {
        WCHAR buf[MAX_PATH + 1] = {0};
        if ( SUCCEEDED(::SHGetFolderPathW(0, CSIDL_PROFILE | CSIDL_FLAG_CREATE, 0, 0, buf)) ) {
            dst = make_utf8(buf);
            return true;
        }
        return false;
    }

    bool extract_appdata_directory(str& dst) {
        WCHAR buf[MAX_PATH + 1] = {0};
        if ( SUCCEEDED(::SHGetFolderPathW(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, 0, 0, buf)) ) {
            dst = make_utf8(buf);
            return true;
        }
        return false;
    }

    bool extract_desktop_directory(str& dst) {
        WCHAR buf[MAX_PATH + 1] = {0};
        if ( SUCCEEDED(::SHGetFolderPathW(0, CSIDL_DESKTOP | CSIDL_FLAG_CREATE, 0, 0, buf)) ) {
            dst = make_utf8(buf);
            return true;
        }
        return false;
    }

    bool extract_working_directory(str& dst) {
        WCHAR buf[MAX_PATH + 1] = {0};
        const DWORD len = ::GetCurrentDirectoryW(
            math::numeric_cast<DWORD>(std::size(buf) - 1),
            buf);
        if ( len > 0 && len <= MAX_PATH ) {
            dst = make_utf8(buf);
            return true;
        }
        return false;
    }

    bool extract_documents_directory(str& dst) {
        WCHAR buf[MAX_PATH + 1] = {0};
        if ( SUCCEEDED(::SHGetFolderPathW(0, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, 0, 0, buf)) ) {
            dst = make_utf8(buf);
            return true;
        }
        return false;
    }

    bool extract_executable_path(str& dst) {
        WCHAR buf[MAX_PATH + 1] = {0};
        const DWORD len = ::GetModuleFileNameW(
            0,
            buf,
            math::numeric_cast<DWORD>(std::size(buf) - 1));
        if ( len > 0 && len <= MAX_PATH ) {
            dst = make_utf8(buf);
            return true;
        }
        return false;
    }

    bool extract_resources_directory(str& dst) {
        str executable_path;
        if ( extract_executable_path(executable_path) ) {
            dst = path::parent_path(executable_path);
            return true;
        }
        return false;
    }
    
    void safe_register_predef_path(
        vfs& the_vfs,
        str_view scheme,
        bool (*extract_directory)(str& dst)) {
        str path;
        if ( extract_directory(path) ) {
            the_vfs.register_scheme_alias(scheme, url{"file", path});
        }
    }

    void platform_internal_state_windows::register_scheme_aliases(vfs& the_vfs) {
        the_vfs.register_scheme<filesystem_file_source>("file");
        safe_register_predef_path(the_vfs, "home", extract_home_directory);
        safe_register_predef_path(the_vfs, "appdata", extract_appdata_directory);
        safe_register_predef_path(the_vfs, "desktop", extract_desktop_directory);
        safe_register_predef_path(the_vfs, "working", extract_working_directory);
        safe_register_predef_path(the_vfs, "documents", extract_documents_directory);
        safe_register_predef_path(the_vfs, "resources", extract_resources_directory);
        safe_register_predef_path(the_vfs, "executable", extract_executable_path);
    }
}

namespace e2d
{
    platform::platform(int argc, char *argv[])
    : state_(new platform_internal_state_windows(argc, argv)) {}
}

int main(int argc, char *argv[]) {
    return e2d_main(argc, argv);
}

#endif
