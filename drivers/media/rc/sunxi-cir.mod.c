#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=rc-core";

MODULE_ALIAS("of:N*T*Callwinner,sun4i-a10-ir");
MODULE_ALIAS("of:N*T*Callwinner,sun4i-a10-irC*");
MODULE_ALIAS("of:N*T*Callwinner,sun5i-a13-ir");
MODULE_ALIAS("of:N*T*Callwinner,sun5i-a13-irC*");
