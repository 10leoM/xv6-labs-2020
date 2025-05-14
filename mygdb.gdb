# init.gdb
file kernel/kernel
b ukvmcopy
b scheduler
c
layout src
