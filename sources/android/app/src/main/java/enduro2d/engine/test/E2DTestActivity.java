/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

package enduro2d.engine.test;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import java.io.File;

public class E2DTestActivity
        extends enduro2d.engine.E2DActivity
{
    private boolean initialized_ = false;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        try {
            Intent params = getIntent();
            String libname = params.getStringExtra(E2DTestApplication.NATIVE_LIB);
            Log.i(TAG, "onCreate: " + libname);
            System.loadLibrary(libname);
            initialized_ = true;
            create();
        } catch (Throwable e) {
            Log.e(TAG, "failed to initialize native application");
            finish();
        }
    }

    @Override protected void onDestroy() {
        Log.i(TAG, "onDestroy");
        super.onDestroy();
        if ( initialized_ ) {
            E2DTestApplication.onDestroyActivity(getApplicationContext());
        }
    }

    @Override protected void onPause() {
        Log.i(TAG, "onPause");
        super.onPause();
    }

    @Override protected void onResume() {
        Log.i(TAG, "onResume");
        super.onResume();
    }

    @Override protected void onStart() {
        Log.i(TAG, "onStart");
        super.onStart();
    }

    @Override protected void onStop() {
        Log.i(TAG, "onStop");
        super.onStop();
    }
}
