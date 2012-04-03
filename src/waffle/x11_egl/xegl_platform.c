// Copyright 2012 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xegl_platform.h"

#include <dlfcn.h>
#include <stdlib.h>

#include <waffle/native.h>
#include <waffle/waffle_enum.h>
#include <waffle/core/wcore_error.h>

#include "xegl_config.h"
#include "xegl_context.h"
#include "xegl_display.h"
#include "xegl_gl_misc.h"
#include "xegl_priv_egl.h"
#include "xegl_priv_types.h"
#include "xegl_window.h"

static const struct native_dispatch xegl_dispatch = {
    .display_connect = xegl_display_connect,
    .display_disconnect = xegl_display_disconnect,
    .config_choose = xegl_config_choose,
    .config_destroy = xegl_config_destroy,
    .context_create = xegl_context_create,
    .context_destroy = xegl_context_destroy,
    .window_create = xegl_window_create,
    .window_destroy = xegl_window_destroy,
    .window_swap_buffers = xegl_window_swap_buffers,
    .make_current = xegl_make_current,
    .get_proc_address = xegl_get_proc_address,
    .dlsym_gl = xegl_dlsym_gl,
};

union native_platform*
xegl_platform_create(
        int gl_api,
        const struct native_dispatch **dispatch)
{
    bool ok = true;

    union native_platform *self;
    NATIVE_ALLOC(self, xegl);
    if (!self) {
        wcore_error(WAFFLE_OUT_OF_MEMORY);
        return NULL;
    }

    self->xegl->gl_api = gl_api;

    switch (gl_api) {
        case WAFFLE_GL:     self->xegl->libgl_name = "libGL.so";        break;
        case WAFFLE_GLES1:  self->xegl->libgl_name = "libGLESv1_CM.so"; break;
        case WAFFLE_GLES2:  self->xegl->libgl_name = "libGLESv2.so";    break;
        default:
            wcore_error_internal("gl_api has bad value 0x%x", gl_api);
            goto error;
    }

    ok &= egl_bind_api(gl_api);
    if (!ok)
        goto error;

    self->xegl->libgl = dlopen(self->xegl->libgl_name, RTLD_LAZY);
    if (!self->xegl->libgl) {
        wcore_errorf(WAFFLE_UNKNOWN_ERROR,
                      "dlopen(\"%s\") failed", self->xegl->libgl_name);
        goto error;
    }

    *dispatch = &xegl_dispatch;
    return self;

error:
    WCORE_ERROR_DISABLED({
        xegl_platform_destroy(self);
    });
    return NULL;
}

bool
xegl_platform_destroy(union native_platform *self)
{
    int error = 0;

    if (!self)
        return true;

    if (self->xegl->libgl) {
        error |= dlclose(self->xegl->libgl);
        if (error) {
            wcore_errorf(WAFFLE_UNKNOWN_ERROR, "dlclose() failed on \"%s\"",
                         self->xegl->libgl_name);
        }
    }

    free(self);
    return !error;
}
