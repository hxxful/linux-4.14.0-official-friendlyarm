cmd_arch/arm64/lib/lib-ksyms.o := aarch64-linux-gnu-objdump -h arch/arm64/lib/lib.a | sed -ne '/___ksymtab/s/.*+\([^ ]*\).*/EXTERN(\1)/p' >arch/arm64/lib/.lib-ksyms.o.lds; rm -f arch/arm64/lib/.lib_exports.o; echo | aarch64-linux-gnu-gcc -Wp,-MD,arch/arm64/lib/.lib-ksyms.o.d  -nostdinc -isystem /root/Workspace/tools/toolchains/gcc-linaro-6.4.1-2017.11-x86_64_aarch64-linux-gnu/bin/../lib/gcc/aarch64-linux-gnu/6.4.1/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -D__ASSEMBLY__ -fno-PIE -DCONFIG_AS_LSE=1 -mabi=lp64 -DCC_HAVE_ASM_GOTO   -c -o arch/arm64/lib/.lib_exports.o -x assembler -; aarch64-linux-gnu-ld -EL  -maarch64linux   -r -o arch/arm64/lib/lib-ksyms.o -T arch/arm64/lib/.lib-ksyms.o.lds arch/arm64/lib/.lib_exports.o; rm arch/arm64/lib/.lib_exports.o arch/arm64/lib/.lib-ksyms.o.lds
