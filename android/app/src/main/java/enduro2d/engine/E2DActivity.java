/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package enduro2d.engine;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

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
        Log.e(TAG, "onCreate");

        opengl_view_ = new SurfaceView(this);
        setContentView(opengl_view_);

        E2DNativeLib.create(this);

        SurfaceHolder holder = opengl_view_.getHolder();
        holder.addCallback(this);
        holder.setFormat(android.graphics.PixelFormat.RGBA_8888);

        opengl_view_.setFocusable(true);
        opengl_view_.setFocusableInTouchMode(true);
        opengl_view_.requestFocus();
        opengl_view_.setOnKeyListener(this);
        opengl_view_.setOnTouchListener(this);
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        Log.e(TAG, "onDestroy");
        E2DNativeLib.destroy();
    }

    @Override protected void onPause() {
        super.onPause();
        Log.e(TAG, "onPause");
        E2DNativeLib.pause();
    }

    @Override protected void onResume() {
        super.onResume();
        Log.e(TAG, "onResume");
        E2DNativeLib.resume();
    }

    @Override protected void onStart() {
        super.onStart();
        Log.e(TAG, "onStart");
        E2DNativeLib.start();
    }

    @Override protected void onStop() {
        super.onStop();
        Log.e(TAG, "onStop");
        E2DNativeLib.stop();
    }

    // SurfaceHolder.Callback
    @Override public final void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.e(TAG, "surfaceChanged");
        E2DNativeLib.surfaceChanged(holder.getSurface(), w, h);
    }

    @Override public final void surfaceCreated(SurfaceHolder holder) {
        Log.e(TAG, "surfaceCreated");
    }

    @Override public final void surfaceDestroyed(SurfaceHolder holder) {
        Log.e(TAG, "surfaceDestroyed");
    }

    // View.OnKeyListener
    @Override public final boolean onKey(View v, int keyCode, KeyEvent ev) {
        Log.e(TAG, "onKey");
        return false;
    }

    // View.OnTouchListener
    @Override public final boolean onTouch(View vw, MotionEvent ev) {
        Log.e(TAG, "onTouch");
        return true;
    }

    private SurfaceView opengl_view_;
}
