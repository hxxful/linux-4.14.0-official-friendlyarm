cmd_drivers/bluetooth/btsdio.ko := aarch64-linux-gnu-ld -EL -r  -maarch64linux -T ./scripts/module-common.lds --build-id  -o drivers/bluetooth/btsdio.ko drivers/bluetooth/btsdio.o drivers/bluetooth/btsdio.mod.o ;  true