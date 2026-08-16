/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib
  Authors: Bernard Teo

  This header contains a function to convert an SDL window handle to a native window handle for
  passing to NFDe.

  This is meant to be used with SDL2, but if there are incompatibilities with future SDL versions,
  we can conditionally compile based on SDL_MAJOR_VERSION.
 */

#ifndef _NFD_SDL2_H
#define _NFD_SDL2_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <nfd.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define NFD_INLINE inline
#else
#define NFD_INLINE static inline
#endif  // __cplusplus

/**
 * Sets the wayland display if the process is running under Wayland, otherwise does nothing.
 * @param sdlWindow The SDL window handle.
 * @return Either true to indicate success, or false to indicate failure.  If false is returned,
 * you can call SDL_GetError() for more information.
 */
NFD_INLINE bool NFD_SetDisplayPropertiesFromSDLWindow(SDL_Window* sdlWindow) {
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
    SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
    if (props == 0) {
        return false;
    }

    void* wl_display =
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);

    // Not a Wayland window -> nothing to do
    if (!wl_display) {
        return true;
    }

    return NFD_SetWaylandDisplay(wl_display) == NFD_OKAY;
#else
    (void)sdlWindow;
    return true;
#endif
}

/**
 * Converts an SDL window handle to a native window handle that can be passed to NFDe.
 * @param sdlWindow The SDL window handle.
 * @param[out] nativeWindow The output native window handle, populated if and only if this function
 * returns true.
 * @return Either true to indicate success, or false to indicate failure.  If false is returned,
 * you can call SDL_GetError() for more information.  However, it is intended that users ignore the
 * error and simply pass a value-initialized nfdwindowhandle_t to NFDe if this function fails. */
NFD_INLINE bool NFD_GetNativeWindowFromSDLWindow(SDL_Window* sdlWindow,
                                                 nfdwindowhandle_t* nativeWindow) {
    SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
    if (props == 0) {
        return false;
    }

#if defined(SDL_VIDEO_DRIVER_WINDOWS)
    void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (hwnd) {
        nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
        nativeWindow->handle = hwnd;
        return true;
    }
#endif

#if defined(SDL_VIDEO_DRIVER_COCOA)
    void* nswindow = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (nswindow) {
        nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_COCOA;
        nativeWindow->handle = nswindow;
        return true;
    }
#endif

#if defined(SDL_VIDEO_DRIVER_X11)
    Uint64 x11_window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    if (x11_window != 0) {
        nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_X11;
        nativeWindow->handle = (void*)(uintptr_t)x11_window;
        return true;
    }
#endif

#if defined(SDL_VIDEO_DRIVER_WAYLAND)
    void* wl_surface =
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    if (wl_surface) {
        nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_WAYLAND;
        nativeWindow->handle = wl_surface;
        return true;
    }
#endif

    SDL_SetError("Unsupported native window type.");
    return false;
}

#undef NFD_INLINE
#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _NFD_SDL2_H
