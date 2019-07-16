/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

package enduro2d.engine;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
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

public class E2DActivity
        extends Activity
        implements SurfaceHolder.Callback,
                    View.OnKeyListener,
                    View.OnTouchListener
{
    private	static final String TAG = "Enduro2D";

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        try {
            Intent params = getIntent();
            String libname = params.getStringExtra(E2DApplication.NATIVE_LIB);
            Log.i(TAG, "onCreate: " + libname);
            System.loadLibrary(libname);
        } catch (Throwable e) {
            Log.e(TAG, "failed to initialize native application");
            finish();
            return;
        }

        opengl_view_ = new SurfaceView(this);
        setContentView(opengl_view_);

        E2DNativeLib.createPlatform(getApplicationContext(), getResources().getAssets());
        E2DNativeLib.createWindow(this);
        sendVersionInfo();
        sendDisplayInfo();

        SurfaceHolder holder = opengl_view_.getHolder();
        holder.addCallback(this);

        opengl_view_.setFocusable(true);
        opengl_view_.setFocusableInTouchMode(true);
        opengl_view_.requestFocus();
        opengl_view_.setOnKeyListener(this);
        opengl_view_.setOnTouchListener(this);
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "onDestroy");
        if ( opengl_view_ != null ) {
            E2DNativeLib.destroyWindow();
            E2DNativeLib.destroyPlatform();
            E2DApplication.onDestroyActivity(getApplicationContext());
        }
    }

    @Override protected void onPause() {
        super.onPause();
        Log.i(TAG, "onPause");
        E2DNativeLib.pause();
    }

    @Override protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume");
        E2DNativeLib.resume();
    }

    @Override protected void onStart() {
        super.onStart();
        Log.i(TAG, "onStart");
        E2DNativeLib.start();
        native_tick_.run();
    }

    @Override protected void onStop() {
        super.onStop();
        Log.i(TAG, "onStop");
        E2DNativeLib.stop();
        native_tick_.run();
        handler_.removeCallbacks(native_tick_);
    }

    @Override public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Display display = ((WindowManager)getSystemService(WINDOW_SERVICE)).getDefaultDisplay();
        E2DNativeLib.orientationChanged(display.getRotation());
    }

    // SurfaceHolder.Callback
    @Override public final void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        E2DNativeLib.surfaceChanged(holder.getSurface(), w, h);
    }

    @Override public final void surfaceCreated(SurfaceHolder holder) {
    }

    @Override public final void surfaceDestroyed(SurfaceHolder holder) {
        E2DNativeLib.surfaceDestroyed();
    }

    // View.OnKeyListener
    @Override public final boolean onKey(View v, int keyCode, KeyEvent ev) {
        if ( keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
             keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
             keyCode == KeyEvent.KEYCODE_VOLUME_MUTE ) {
            // android will change volume
            return false;
        }
        if ( keyCode == KeyEvent.KEYCODE_BACK ) {
            return false;
        }
        E2DNativeLib.onKey(keyCode, ev.getAction());
        return true;
    }

    // View.OnTouchListener
    @Override public final boolean onTouch(View vw, MotionEvent ev) {
        int num_pointers = Math.min(ev.getPointerCount(), max_touches_);
        int action = ev.getActionMasked();
        if ( action == MotionEvent.ACTION_MOVE && num_pointers > 1 ) {
            for ( int i = 0, j = 0; i < num_pointers; ++i ) {
                touch_data_[j++] = (float)ev.getPointerId(i);
                touch_data_[j++] = ev.getX(i);
                touch_data_[j++] = ev.getY(i);
                touch_data_[j++] = ev.getPressure(i);
            }
        } else {
            int index = ev.getActionIndex();
            touch_data_[0] = (float)ev.getPointerId(index);
            touch_data_[1] = ev.getX(index);
            touch_data_[2] = ev.getY(index);
            touch_data_[3] = ev.getPressure(index);
        }
        E2DNativeLib.onTouch(action, num_pointers, touch_data_);
        return true;
    }

    // called from native code
    @SuppressWarnings("unused") public AssetManager getAssetManager() {
        return getResources().getAssets();
    }

    @SuppressWarnings("unused") public void setActivityTitle(String value) {
        this.setTitle(value);
    }

    @SuppressWarnings("unused") public void showToast(String msg, boolean long_time) {
        int duration = long_time ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT;
        Toast toast = Toast.makeText(this, msg, duration);
        toast.show();
    }

    @SuppressWarnings("unused") public void SetScreenOrientation(int value) {
        setRequestedOrientation(value);
    }

    @SuppressWarnings("unused") public boolean isNetworkConnected () {
        try {
            ConnectivityManager cm = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo ni = cm.getActiveNetworkInfo();
            if ( ni != null ) {
                NetworkInfo.DetailedState state = ni.getDetailedState();
                if ( state == NetworkInfo.DetailedState.CONNECTED )
                    return true;
            }
        } catch (Exception e) {
            Log.e(TAG, "exception: " + e.toString());
        }
        return false;
    }

    private void sendVersionInfo() {
        E2DNativeLib.setApiVersion(android.os.Build.VERSION.SDK_INT);
    }

    private void sendDisplayInfo() {
        WindowManager wm = (WindowManager)getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics metrics = new DisplayMetrics();
        Display display = wm.getDefaultDisplay();
        display.getMetrics(metrics);
        E2DNativeLib.orientationChanged(display.getRotation());
        E2DNativeLib.setDisplayInfo(metrics.widthPixels, metrics.heightPixels, metrics.densityDpi);
    }

    private Runnable native_tick_ = new Runnable() {
        @Override
        public void run() {
            try {
                E2DNativeLib.tick();
            } catch(Throwable e) {
                String s = "Catched exception: ";
                s += e.toString();
                s += android.util.Log.getStackTraceString(e);
                Log.e(TAG, "exception: " + s);
            }
            handler_.postDelayed(native_tick_, 1000/30);
        }
    };

    private static final int max_touches_ = 8;
    private float[] touch_data_ = new float[max_touches_ * 4]; // packed: {id, x, y, pressure}
    private Handler handler_ = new Handler();
    private SurfaceView opengl_view_;
}
