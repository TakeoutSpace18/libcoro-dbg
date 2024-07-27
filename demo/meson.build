corodemo_src = files(
  'corodemo.c',
  'coro.c'
)

executable('corodemo',
  corodemo_src,
  c_args: ['-DCORO_DEBUG', '-fno-omit-frame-pointer']
)
