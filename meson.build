project('locker', 'c',
  default_options: ['c_std=c11']
)

src_dir = 'src/'
inc_dir = include_directories('include')

wayland_client = dependency('wayland-client', required:true)

deps = [wayland_client]

src_files = [
  src_dir + 'main.c',
  src_dir + 'shm.c',
  src_dir + 'xdg-shell-client-protocol.c'
]

executable(meson.project_name(), src_files, include_directories: inc_dir, dependencies: deps)
