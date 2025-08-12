project('locker', 'c',
  default_options: ['c_std=c11'],
  version: '0.0.1-alpha'
)
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')
# add_project_arguments('-Wall', '-Wextra', '-Wpedantic', language: 'c')

src_dir = 'src'
inc_dir = include_directories('include')

wayland_client_dep = dependency('wayland-client', required:true)
pam_dep = dependency('pam', required: true)
libxkbcommon_dep = dependency('xkbcommon', required: true)
cairo_dep = dependency('cairo', required: true)

deps = [wayland_client_dep, libxkbcommon_dep, cairo_dep, pam_dep]

src_files = files(
  src_dir / 'main.c',
  src_dir / 'shm.c',
  src_dir / 'draw.c',
  src_dir / 'auth.c',
  src_dir / 'ext-session-lock-v1-protocol.c'
)

executable(meson.project_name(), src_files, include_directories: inc_dir, dependencies: deps)
