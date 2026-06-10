/* Nordstjernen — toolkit-independent offscreen GLES context for WebGL.
 * Copyright 2026 Andreas Røsdal
 * SPDX-License-Identifier: LicenseRef-NSL-1.0
 */

#include "glctx.h"

#if defined(NS_ENABLE_WEBGL) && defined(NS_HAVE_EGL)

#include <string.h>

#include <epoxy/egl.h>

struct ns_gl_context {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};

static EGLDisplay
ns_gl_shared_display(void)
{
    static gsize once = 0;
    static EGLDisplay shared = EGL_NO_DISPLAY;
    if (g_once_init_enter(&once)) {
        EGLDisplay d = EGL_NO_DISPLAY;
        const char *exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
        if (exts && strstr(exts, "EGL_MESA_platform_surfaceless")) {
            PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display =
                (PFNEGLGETPLATFORMDISPLAYEXTPROC)
                    eglGetProcAddress("eglGetPlatformDisplayEXT");
            if (get_platform_display)
                d = get_platform_display(EGL_PLATFORM_SURFACELESS_MESA,
                                         EGL_DEFAULT_DISPLAY, NULL);
        }
        if (d == EGL_NO_DISPLAY)
            d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (d != EGL_NO_DISPLAY) {
            EGLint major = 0, minor = 0;
            if (eglInitialize(d, &major, &minor))
                shared = d;
        }
        g_once_init_leave(&once, 1);
    }
    return shared;
}

static gboolean
ns_gl_choose_config(EGLDisplay d, EGLConfig *out)
{
    EGLint n = 0;
    const EGLint pbuffer_attrs[] = {
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE,
    };
    if (eglChooseConfig(d, pbuffer_attrs, out, 1, &n) && n >= 1)
        return TRUE;

    const EGLint any_attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE,
    };
    return eglChooseConfig(d, any_attrs, out, 1, &n) && n >= 1;
}

ns_gl_context *
ns_gl_context_create(void)
{
    EGLDisplay d = ns_gl_shared_display();
    if (d == EGL_NO_DISPLAY) return NULL;
    if (!eglBindAPI(EGL_OPENGL_ES_API)) return NULL;

    EGLConfig config;
    if (!ns_gl_choose_config(d, &config)) return NULL;

    EGLint ctx_attrs[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE };
    EGLContext context = eglCreateContext(d, config, EGL_NO_CONTEXT, ctx_attrs);
    if (context == EGL_NO_CONTEXT) {
        ctx_attrs[1] = 2;
        context = eglCreateContext(d, config, EGL_NO_CONTEXT, ctx_attrs);
    }
    if (context == EGL_NO_CONTEXT) return NULL;

    EGLSurface surface = EGL_NO_SURFACE;
    if (!eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        const EGLint pb_attrs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
        surface = eglCreatePbufferSurface(d, config, pb_attrs);
        if (surface == EGL_NO_SURFACE ||
            !eglMakeCurrent(d, surface, surface, context)) {
            if (surface != EGL_NO_SURFACE) eglDestroySurface(d, surface);
            eglDestroyContext(d, context);
            return NULL;
        }
    }

    ns_gl_context *c = g_new0(ns_gl_context, 1);
    c->display = d;
    c->context = context;
    c->surface = surface;
    return c;
}

gboolean
ns_gl_context_make_current(ns_gl_context *c)
{
    if (!c) return FALSE;
    return eglMakeCurrent(c->display, c->surface, c->surface, c->context)
               ? TRUE : FALSE;
}

void
ns_gl_context_release(ns_gl_context *c)
{
    if (!c) return;
    eglMakeCurrent(c->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void
ns_gl_context_destroy(ns_gl_context *c)
{
    if (!c) return;
    eglMakeCurrent(c->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (c->surface != EGL_NO_SURFACE) eglDestroySurface(c->display, c->surface);
    if (c->context != EGL_NO_CONTEXT) eglDestroyContext(c->display, c->context);
    g_free(c);
}

#elif defined(NS_ENABLE_WEBGL)

ns_gl_context *ns_gl_context_create(void) { return NULL; }
gboolean ns_gl_context_make_current(ns_gl_context *c) { (void)c; return FALSE; }
void ns_gl_context_release(ns_gl_context *c) { (void)c; }
void ns_gl_context_destroy(ns_gl_context *c) { (void)c; }

#endif /* NS_ENABLE_WEBGL */
