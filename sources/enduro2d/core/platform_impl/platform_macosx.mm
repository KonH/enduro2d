/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_MACOSX

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#import <Foundation/Foundation.h>

namespace
{
    using namespace e2d;

    class platform_internal_state_macosx final : public platform::internal_state {
    public:
        platform_internal_state_macosx(int argc, char** argv)
        : internal_state(argc, argv) {}

        ~platform_internal_state_macosx() noexcept = default;

        void register_scheme_aliases(vfs&) override;
    };

    bool extract_home_directory(str& dst) {
        NSString* path = NSHomeDirectory();
        if ( path ) {
            dst.assign([path UTF8String]);
            return true;
        }
        return false;
    }

    bool extract_appdata_directory(str& dst) {
        NSArray<NSString*>* paths = NSSearchPathForDirectoriesInDomains(
            NSApplicationSupportDirectory, NSUserDomainMask, YES);
        if ( paths && paths.count > 0 ) {
            dst.assign([[paths objectAtIndex:0] UTF8String]);
            return true;
        }
        return false;
    }

    bool extract_desktop_directory(str& dst) {
        NSArray<NSString*>* paths = NSSearchPathForDirectoriesInDomains(
            NSDesktopDirectory, NSUserDomainMask, YES);
        if ( paths && paths.count > 0 ) {
            dst.assign([[paths objectAtIndex:0] UTF8String]);
            return true;
        }
        return false;
    }

    bool extract_working_directory(str& dst) {
        NSString* filename = [[NSFileManager defaultManager] currentDirectoryPath];
        if ( filename ) {
            dst.assign([filename UTF8String]);
            return true;
        }
        return false;
    }

    bool extract_documents_directory(str& dst) {
        NSArray<NSString*>* paths = NSSearchPathForDirectoriesInDomains(
            NSDocumentDirectory, NSUserDomainMask, YES);
        if ( paths && paths.count > 0 ) {
            dst.assign([[paths objectAtIndex:0] UTF8String]);
            return true;
        }
        return false;
    }

    bool extract_resources_directory(str& dst) {
        NSString* filename = [[NSBundle mainBundle] resourcePath];
        if ( filename ) {
            dst.assign([filename UTF8String]);
            return true;
        }
        return false;
    }

    bool extract_executable_path(str& dst) {
        NSString* filename = [[NSBundle mainBundle] executablePath];
        if ( filename ) {
            dst.assign([filename UTF8String]);
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
    
    void platform_internal_state_macosx::register_scheme_aliases(vfs& the_vfs) {
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
    : state_(new platform_internal_state_macosx(argc, argv)) {}
}

int main(int argc, char *argv[]) {
    return e2d_main(argc, argv);
}

#endif
