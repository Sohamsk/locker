project('locker', 'c',
  default_options: ['c_std=c11']
)

src_dir = 'src/'
inc_dir = include_directories('include')

wayland_client_dep = dependency('wayland-client', required:true)
libxkbcommon_dep = dependency('xkbcommon', required: true)
cairo_dep = dependency('cairo', required: true)

deps = [wayland_client_dep, libxkbcommon_dep, cairo_dep]

src_files = [
  src_dir + 'main.c',
  src_dir + 'shm.c',
  src_dir + 'draw.c',
  src_dir + 'ext-session-lock-v1-protocol.c'
]

executable(meson.project_name(), src_files, include_directories: inc_dir, dependencies: deps)
