libcorostacks_src = files(
  'libcorostacks.c',
  'coro_states.c',
  'coredump_attach.c',
  'errors.c'
)

libelf_dep = dependency('libelf')
libdw_dep = dependency('libdw')

libcorostacks = library('corostacks',
  libcorostacks_src,
  dependencies : [libelf_dep, libdw_dep],
  install: true,
  include_directories: 'include' 
)

libcorostacks_dep = declare_dependency(
  link_with: libcorostacks,
  include_directories: 'include'
  )

install_headers('include/libcorostacks.h')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : 'libcorostacks',
                 version : '0.0.1',
                 name : 'libcorostacks',
                 filebase : 'libcorostacks',
                 description : 'A Library to examine coroutine stacks')
