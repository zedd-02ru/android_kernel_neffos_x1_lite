#
# Makefile for the kernel mmc core.
#

# [fengyunliang start] EMCP: Add cmd_edge MSDC_SMPL_RISING Feature
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/mmc/host/mediatek/$(MTK_PLATFORM)
# [fengyunliang end]

obj-$(CONFIG_MMC)		+= mmc_core.o
mmc_core-y			:= core.o bus.o host.o \
				   mmc.o mmc_ops.o sd.o sd_ops.o \
				   sdio.o sdio_ops.o sdio_bus.o \
				   sdio_cis.o sdio_io.o sdio_irq.o \
				   quirks.o slot-gpio.o

mmc_core-$(CONFIG_DEBUG_FS)	+= debugfs.o
