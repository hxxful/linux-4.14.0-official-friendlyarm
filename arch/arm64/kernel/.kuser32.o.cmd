cmd_arch/arm64/kernel/kuser32.o := aarch64-linux-gnu-gcc -Wp,-MD,arch/arm64/kernel/.kuser32.o.d  -nostdinc -isystem /root/Workspace/tools/toolchains/gcc-linaro-6.4.1-2017.11-x86_64_aarch64-linux-gnu/bin/../lib/gcc/aarch64-linux-gnu/6.4.1/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -D__ASSEMBLY__ -fno-PIE -DCONFIG_AS_LSE=1 -mabi=lp64 -DCC_HAVE_ASM_GOTO   -c -o arch/arm64/kernel/kuser32.o arch/arm64/kernel/kuser32.S

source_arch/arm64/kernel/kuser32.o := arch/arm64/kernel/kuser32.S

deps_arch/arm64/kernel/kuser32.o := \
  arch/arm64/include/asm/unistd.h \
    $(wildcard include/config/compat.h) \
  arch/arm64/include/uapi/asm/unistd.h \
  include/asm-generic/unistd.h \
  include/uapi/asm-generic/unistd.h \
    $(wildcard include/config/mmu.h) \
  arch/arm64/include/uapi/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
    $(wildcard include/config/64bit.h) \
  include/uapi/asm-generic/bitsperlong.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/module/rel/crcs.h) \
    $(wildcard include/config/trim/unused/ksyms.h) \
    $(wildcard include/config/unused/symbols.h) \

arch/arm64/kernel/kuser32.o: $(deps_arch/arm64/kernel/kuser32.o)

$(deps_arch/arm64/kernel/kuser32.o):
