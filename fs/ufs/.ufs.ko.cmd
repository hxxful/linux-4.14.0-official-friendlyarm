cmd_fs/ufs/ufs.ko := aarch64-linux-gnu-ld -EL -r  -maarch64linux -T ./scripts/module-common.lds --build-id  -o fs/ufs/ufs.ko fs/ufs/ufs.o fs/ufs/ufs.mod.o ;  true
