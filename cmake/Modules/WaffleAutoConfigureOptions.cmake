# Copyright 2014 Kenney Phillis
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
if(waffle_on_linux)
	find_package(PkgConfig)
	pkg_check_modules(EGL egl)
	if(EGL_FOUND)
		set(waffle_has_egl true)
	endif()
	pkg_check_modules(GL gl)
    pkg_check_modules(X11-XCB x11-xcb)
	pkg_check_modules(X11 x11)
	if(X11_FOUND AND X11-XCB_FOUND)
		set(waffle_has_x11 true)
		if(GL_FOUND)
			set(waffle_has_glx true)
		endif()
	endif()
    pkg_check_modules(GBM REQUIRED gbm)
    pkg_check_modules(LIBUDEV REQUIRED libudev)
	if(LIBUDEV_FOUND AND GBM_FOUND)
		set(waffle_has_gbm true)
	endif()
	set(WAYLAND_CLIENT_MINVER "1")
	set(WAYLAND_EGL_MINVER "9.1")
    pkg_check_modules(WAYLAND_CLIENT wayland-client)
    pkg_check_modules(WAYLAND_EGL wayland-egl)
	if(WAYLAND_CLIENT_FOUND AND WAYLAND_EGL_FOUND)
		if(WAYLAND_EGL_VERSION VERSION_LESS WAYLAND_EGL_MINVER OR
			WAYLAND_CLIENT_VERSION VERSION_LESS WAYLAND_CLIENT_MINVER
			)
			# do not enable wayland unless minimum version is met.
			set(waffle_has_wayland false)
		else()
			set(waffle_has_wayland true)
		endif()

	endif()
elseif(waffle_on_mac)
	# TODO: Does mac need any auto-configuration?
endif()
