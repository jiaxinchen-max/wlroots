# libtermux-render and termux/render/* (render.h, buffer.h, tlog) are on system include path.
# libtermux-render uses EGL (e.g. eglDestroyImageKHR). When tinywl loads it, the process must
# have libEGL loaded; termux-render-sample works because it links EGL. So we pull in EGL here
# so any executable linking wlroots (e.g. tinywl) gets DT_NEEDED libEGL and the symbol resolves.
cc = meson.get_compiler('c')
termux_dep = cc.find_library('termux-render', required: true)
termux_egl = dependency('egl', required: true)
termux_inc = include_directories('.')

wlr_deps += [termux_dep, termux_egl, declare_dependency(include_directories: termux_inc)]
wlr_files += files(
  'backend.c',
  'input.c',
  'output.c',
  'termux_render.c',
)
