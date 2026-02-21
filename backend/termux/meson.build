# libtermux-render and termux/render/* (render.h, buffer.h, tlog) are on system include path.
# libtermux-render uses EGL (e.g. eglDestroyImageKHR). Termux has no libEGL; link Android system libEGL.
cc = meson.get_compiler('c')
termux_dep = cc.find_library('termux-render', required: true)
termux_egl = declare_dependency(
	link_args: [
		'-L/system/lib64', '-lEGL',
		'-Wl,-rpath,/system/lib64:/system/lib',
	],
)
termux_inc = include_directories('.')

wlr_deps += [termux_dep, termux_egl, declare_dependency(include_directories: termux_inc)]
wlr_files += files(
  'backend.c',
  'input.c',
  'output.c',
  'termux_render.c',
)
