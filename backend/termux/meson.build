# libtermux-render and termux/render/* (render.h, buffer.h, tlog) are on system include path.
# libtermux-render uses EGL (e.g. eglDestroyImageKHR). Pull in wayland-egl so the process
# gets libwayland-egl (and its dependency libEGL) loaded, then the symbol resolves.
cc = meson.get_compiler('c')
termux_dep = cc.find_library('termux-render', required: true)
termux_egl = dependency('wayland-egl', required: true)
termux_inc = include_directories('.')

wlr_deps += [termux_dep, termux_egl, declare_dependency(include_directories: termux_inc)]
wlr_files += files(
  'backend.c',
  'input.c',
  'output.c',
  'termux_render.c',
)
