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

public class E2DNativeLib {
     // activity life-cycle
     public static native void create(Object activity);
     public static native void destroy();
     public static native void start();
     public static native void stop();
     public static native void pause();
     public static native void resume();

     public static native void tick();

     // surface
     public static native void surfaceChanged(Object surface, int width, int height);
     public static native void surfaceDestroyed();

     public static native void visibilityChanged();
     public static native void orientationChanged(int newOrientation);

     // input
     public static native void onKey(int keycode, int action);
     public static native void onTouch(int action, int num_pointers, float[] data);

     public static native void onLowMemory();
     public static native void onTrimMemory();

     // display, device ... info
     public static native void setApiVersion(int version);
     public static native void setDisplayInfo(int w, int h, int ppi);
}
