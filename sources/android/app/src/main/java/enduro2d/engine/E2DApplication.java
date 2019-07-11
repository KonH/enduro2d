/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

package enduro2d.engine;

import android.app.Application;
import android.content.Context;
import android.content.Intent;

public final class E2DApplication
    extends Application
{
    public final static String NATIVE_LIB = "NativeLib";

    private final static String[] native_app_list_ = {
            /*"untests_base",
            "untests_core",
            "untests_math",
            "untests_utils",*/
            //"untests_high",
            "sample_00",
            "sample_01",
            "sample_02",
            "sample_03",
            "sample_04",
            "sample_05"
    };
    private static int native_app_index_ = 0;

    @Override public final void onCreate() {
        super.onCreate();
        startApp(native_app_list_[native_app_index_++]);
    }

    public static void onDestroyActivity(Context ctx) {
        if ( native_app_index_ < native_app_list_.length ) {
            ((E2DApplication) ctx).startApp(native_app_list_[native_app_index_++]);
        }
    }

    private void startApp(String libname) {
        Intent intent = new Intent(this, E2DActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        intent.putExtra(NATIVE_LIB, libname);
        startActivity(intent);
    }
}

