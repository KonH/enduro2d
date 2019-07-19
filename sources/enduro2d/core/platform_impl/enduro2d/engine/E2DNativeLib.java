/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

package enduro2d.engine;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public final class E2DNativeLib {
     // application
     public static native void createPlatform(Object ctx, Object asset_mngr);
     public static native void destroyPlatform();
     public static native void setPredefPath(String internal_appdata,
                                           String internal_cache,
                                           String external_appdata,
                                           String external_cache,
                                           String external_storage);

     // activity life-cycle
     public static native void createWindow(Object activity);
     public static native void destroyWindow();
     public static native void start();
     public static native void stop();
     public static native void pause();
     public static native void resume();

     public static native void tick();

     // surface
     public static native void surfaceChanged(Object surface);
     public static native void surfaceDestroyed();

     public static native void visibilityChanged();
     public static native void orientationChanged(int newOrientation);

     // input
     public static native void onKey(int keycode, int action);
     public static native void onTouch(int action, int num_pointers, float[] data);

     public static native void onLowMemory();
     public static native void onTrimMemory();

     // display, device ... info
     public static native void setDisplayInfo(int w, int h, int ppi);

     // called from native code
     @SuppressWarnings("unused") private static int nativeMethodCount() {
          Method[] m = E2DNativeLib.class.getDeclaredMethods();
          int count = 0;
          for (int i = 0; i < m.length; ++i) {
               count += ((m[i].getModifiers() & Modifier.NATIVE) == Modifier.NATIVE ? 1 : 0);
          }
          return count;
     }
}
