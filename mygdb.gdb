# init.gdb
file kernel/kernel
b allocproc
b proc_freekpagetable
b scheduler
c
layout src
