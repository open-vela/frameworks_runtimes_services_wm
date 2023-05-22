/*
 * Copyright (C) 2023 Xiaomi Corporation
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

package os.wm;

import os.wm.DisplayInfo;
import os.wm.InputChannel;
import os.wm.IWindow;
import os.wm.LayerState;
import os.wm.LayoutParams;
import os.wm.SurfaceControl;

interface IWindowManager {
    int getPhysicalDisplayInfo(int displayId, out DisplayInfo info);

    int addWindow(IWindow window, in LayoutParams attrs, in int visibility, 
                in int displayId, in int userId, out InputChannel outInputChannel);

    void removeWindow(IWindow window);

    /**
     * Change the parameters of a window.
     * @param window The window being modified.
     * @param attrs If non-null, new attributes to apply to the window.
     * @param requestedWidth The width the window wants to be.
     * @param requestedHeight The height the window wants to be.
     * @param visibility Window root view's visibility.
     * @param outSurfaceControl Object in which is placed the new display surface.
     */
    int relayout(IWindow window, in LayoutParams attrs, int requestedWidth, 
                int requestedHeight, int visibility, out SurfaceControl outSurfaceControl);


    /** Returns {@code true} if this binder is a registered window token. */
    boolean isWindowToken(in IBinder binder);

    /**
     * Adds window token for a given type.
     *
     * @param token Token to be registered.
     * @param type Window type to be used with this token.
     * @param displayId The ID of the display where this token should be added.
     */
    void addWindowToken(IBinder token, int type, int displayId);

    /**
     * Remove window token on a specific display.
     *
     * @param token Token to be removed
     * @displayId The ID of the display where this token should be removed.
     */
    void removeWindowToken(IBinder token, int displayId);

    int applyTransaction(in LayerState[] state);

    void requestNextVsync(IWindow window);
}