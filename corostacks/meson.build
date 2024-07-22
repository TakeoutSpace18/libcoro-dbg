corostacks_src = files(
  'corostacks.c',
  'coro_states.c'
)

libunwind_dep = declare_dependency(
  dependencies: dependency('libunwind'),
  link_args: ['-lunwind-generic', '-lunwind-coredump'])

libelf_dep = dependency('libelf')
libdw_dep = dependency('libdw')

executable('corostacks',
  corostacks_src,
  dependencies : [libunwind_dep, libelf_dep, libdw_dep]
)
