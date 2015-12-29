#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = /home/think/ti/syslink_2_21_01_05/packages;/home/think/ti/bios_6_35_04_50/packages;/home/think/ti/ipc_1_25_03_15/packages;/home/think/ti/xdctools_3_25_03_72/packages;/home/think/ti/mathlib_c674x_3_0_2_0/packages
override XDCROOT = /home/think/ti/xdctools_3_25_03_72
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = /home/think/ti/syslink_2_21_01_05/packages;/home/think/ti/bios_6_35_04_50/packages;/home/think/ti/ipc_1_25_03_15/packages;/home/think/ti/xdctools_3_25_03_72/packages;/home/think/ti/mathlib_c674x_3_0_2_0/packages;/home/think/ti/xdctools_3_25_03_72/packages;..
HOSTOS = Linux
endif
