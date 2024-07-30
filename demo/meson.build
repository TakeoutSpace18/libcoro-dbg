corodemo_src = files(
  'corodemo.c',
  'coro.c'
)

executable('corodemo',
  corodemo_src,
  c_args: ['-DCORO_DEBUG', '-fno-omit-frame-pointer']
)

corobench_src = files(
  'corobench.c',
  'coro.c'
)

# libcoro with state table
executable('corobench-st',
  corobench_src,
  c_args: ['-DCORO_DEBUG', '-fno-omit-frame-pointer']
)

# vanilla libcoro 
executable('corobench-no-st',
  corobench_src,
  c_args: ['-fno-omit-frame-pointer']
)
