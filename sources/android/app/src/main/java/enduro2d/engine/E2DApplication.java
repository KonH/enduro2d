/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

package enduro2d.engine;

import android.app.Application;

public final class E2DApplication
    extends Application
{
    static {
        System.loadLibrary("sample_00");
    }

    @Override public final void onCreate() {
        super.onCreate();
        E2DNativeLib.initialize(this, getResources().getAssets());
    }
}