cmd_drivers/usb/mon/usbmon.ko := aarch64-linux-gnu-ld -EL -r  -maarch64linux -T ./scripts/module-common.lds --build-id  -o drivers/usb/mon/usbmon.ko drivers/usb/mon/usbmon.o drivers/usb/mon/usbmon.mod.o ;  true