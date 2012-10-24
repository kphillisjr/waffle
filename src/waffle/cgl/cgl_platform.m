// Copyright 2012 Intel Corporation
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>

#include "wcore_error.h"

#include "cgl_config.h"
#include "cgl_context.h"
#include "cgl_display.h"
#include "cgl_dl.h"
#include "cgl_platform.h"
#include "cgl_window.h"

static const struct wcore_platform_vtbl cgl_platform_vtbl;

static bool
cgl_platform_destroy(struct wcore_platform *wc_self)
{
    struct cgl_platform *self = cgl_platform(wc_self);
    bool ok = true;

    if (!self)
        return ok;

    if (self->dl_gl)
        ok &= cgl_dl_close(&self->wcore);

    ok &= wcore_platform_teardown(wc_self);
    free(self);
    return ok;
}

struct wcore_platform*
cgl_platform_create(void)
{
    struct cgl_platform *self;
    bool ok = true;

    self= calloc(1, sizeof(*self));
    if (!self) {
        wcore_error(WAFFLE_ERROR_BAD_ALLOC);
        return NULL;
    }

    ok = wcore_platform_init(&self->wcore);
    if (!ok)
        goto error;

    self->wcore.vtbl = &cgl_platform_vtbl;
    return &self->wcore;

error:
    cgl_platform_destroy(&self->wcore);
    return NULL;
}

static bool
cgl_make_current(struct wcore_platform *wc_self,
                          struct wcore_display *wc_dpy,
                          struct wcore_window *wc_window,
                          struct wcore_context *wc_ctx)
{
    struct cgl_window *window = cgl_window(wc_window);
    struct cgl_context *ctx = cgl_context(wc_ctx);

    NSOpenGLContext *cocoa_ctx = ctx ? ctx->ns : NULL;
    WaffleGLView* cocoa_view = window ? window->gl_view : NULL;

    if (cocoa_view)
        cocoa_view.glContext = cocoa_ctx;

    if (cocoa_ctx) {
        [cocoa_ctx makeCurrentContext];
        [cocoa_ctx setView:cocoa_view];
    }

    return true;
}

static void*
cgl_get_proc_address(struct wcore_platform *wc_self,
                              const char *name)
{
    // There is no CGLGetProcAddress.
    return NULL;
}

static const struct wcore_platform_vtbl cgl_platform_vtbl = {
    .destroy = cgl_platform_destroy,

    .make_current = cgl_make_current,
    .get_proc_address = cgl_get_proc_address,
    .dl_can_open = cgl_dl_can_open,
    .dl_sym = cgl_dl_sym,

    .display = {
        .connect = cgl_display_connect,
        .destroy = cgl_display_destroy,
        .supports_context_api = cgl_display_supports_context_api,
        .get_native = NULL,
    },

    .config = {
        .choose = cgl_config_choose,
        .destroy = cgl_config_destroy,
        .get_native = NULL,
    },

    .context = {
        .create = cgl_context_create,
        .destroy = cgl_context_destroy,
        .get_native = NULL,
    },

    .window = {
        .create = cgl_window_create,
        .destroy = cgl_window_destroy,
        .show = cgl_window_show,
        .swap_buffers = cgl_window_swap_buffers,
        .get_native = NULL,
    },
};
