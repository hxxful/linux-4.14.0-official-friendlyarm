cmd_fs/ncpfs/ncpsign_kernel.o := aarch64-linux-gnu-gcc -Wp,-MD,fs/ncpfs/.ncpsign_kernel.o.d  -nostdinc -isystem /root/Workspace/tools/toolchains/gcc-linaro-4.9-2015.02-3-x86_64_aarch64-linux-gnu/bin/../lib/gcc/aarch64-linux-gnu/4.9.3/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 -fno-PIE -mgeneral-regs-only -DCONFIG_BROKEN_GAS_INST=1 -fno-asynchronous-unwind-tables -mabi=lp64 -fno-delete-null-pointer-checks -O2 --param=allow-store-data-races=0 -DCC_HAVE_ASM_GOTO -Wframe-larger-than=2048 -fno-stack-protector -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time  -DMODULE -mcmodel=large  -DKBUILD_BASENAME='"ncpsign_kernel"'  -DKBUILD_MODNAME='"ncpfs"' -c -o fs/ncpfs/ncpsign_kernel.o fs/ncpfs/ncpsign_kernel.c

source_fs/ncpfs/ncpsign_kernel.o := fs/ncpfs/ncpsign_kernel.c

deps_fs/ncpfs/ncpsign_kernel.o := \
    $(wildcard include/config/ncpfs/packet/signing.h) \

fs/ncpfs/ncpsign_kernel.o: $(deps_fs/ncpfs/ncpsign_kernel.o)

$(deps_fs/ncpfs/ncpsign_kernel.o):
