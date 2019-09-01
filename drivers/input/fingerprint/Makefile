#
# Makefile for all kinds of Fingerprint
#

obj-$(CONFIG_GOODIX_FINGERPRINT) += goodix/
obj-$(CONFIG_FPC_FINGERPRINT) += fpc/
# [taoyancheng] Add fingerprint driver for Goodix GX368
ifeq ($(BUILD_TARGET_PRODUCT_TP), tp902)
    obj-$(CONFIG_GOODIX_FINGERPRINT_GX368) += goodix/gx368_old/
else ifeq ($(BUILD_TARGET_PRODUCT_TP), tp903)
    obj-$(CONFIG_GOODIX_FINGERPRINT_GX368) += goodix/gx368_old/
else ifeq ($(BUILD_TARGET_PRODUCT_TP), tp910)
    obj-$(CONFIG_SUNWAVE_FINGERPRINT_SW9651) += sunwavecorp/tee/
else
    obj-$(CONFIG_GOODIX_FINGERPRINT_GX368) += goodix/gx368/
# [liguanxiong start] Add for secondary fingerprint module Sunwave sw9251
    obj-$(CONFIG_SUNWAVE_FINGERPRINT_SW9251) += sunwavecorp/
# [liguanxiong end]
endif
