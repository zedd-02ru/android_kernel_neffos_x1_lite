subdir-ccflags-y += -Werror
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek
# In case the platform does NOT support this type of sensors

ifneq ($(CONFIG_TOUCHSCREEN_MTK),)
obj-y   +=  mtk_tpd.o tpd_button.o tpd_calibrate.o tpd_debug.o \
		tpd_default.o tpd_init.o tpd_misc.o tpd_setting.o
endif

obj-$(CONFIG_TOUCHSCREEN_MTK)   += tpd_common_probe.o
obj-$(CONFIG_TOUCHSCREEN_TPLINK904_GT5668)  +=tplink_904_gt5688/
obj-$(CONFIG_TOUCHSCREEN_TPLINK904_FT8607)	+=tplink_904_ft8607/
obj-$(CONFIG_TOUCHSCREEN_TPLINK903_FT8716)	+=tplink_903_ft8716/
obj-$(CONFIG_TOUCHSCREEN_TPLINK902_FT8607)	+=tplink_902_ft8607/
obj-$(CONFIG_TOUCHSCREEN_FT5436)	+= ft5436/
