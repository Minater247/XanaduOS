with open('../inc_asm/pagetables.s', 'w') as f:
    f.write('.globl boot_pt1\n')
    f.write('boot_pt1:\n')
    for i in range(1024):
        f.write('.long 0x{:08x}\n'.format((i << 12) | 3))
    f.write('.globl boot_pt2\n')
    f.write('boot_pt2:\n')
    for i in range(1024):
        f.write('.long 0x{:08x}\n'.format(((i + 1024) << 12) | 3))