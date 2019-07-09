/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

package enduro2d.engine;

public class E2DNativeLib {
     // application
     public static native void initialize(Object ctx, Object asset_mngr);

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
