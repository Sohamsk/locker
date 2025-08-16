project('tomlc17', 'c',
  version : 'R250712',
  license : 'MIT',
  default_options : [
    'c_std=c99',
    'warning_level=2'
  ]
)

# tomlc17 is a C library with both static and header-only options
tomlc17_sources = files('src/tomlc17.c')
tomlc17_headers = files('src/tomlc17.h')

# Create the dependency
tomlc17_dep = declare_dependency(
  sources: tomlc17_sources,
  include_directories : include_directories('src/')
)

# Override the dependency name
meson.override_dependency('tomlc17', tomlc17_dep)
