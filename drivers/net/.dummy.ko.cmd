cmd_drivers/net/dummy.ko := aarch64-linux-gnu-ld -EL -r  -maarch64linux -T ./scripts/module-common.lds --build-id  -o drivers/net/dummy.ko drivers/net/dummy.o drivers/net/dummy.mod.o ;  true
