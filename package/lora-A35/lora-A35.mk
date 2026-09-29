################################################################################
#
# lora-A35
#
################################################################################

LORA_A35_VERSION = 1.0

LORA_A35_SITE = \
	$(BR2_EXTERNAL_MAINEXTERNAL_PATH)/package/lora-A35/src

LORA_A35_SITE_METHOD = local

define LORA_A35_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) \
		CC="$(TARGET_CC)" \
		CPPFLAGS="$(TARGET_CPPFLAGS)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		-C $(@D)
endef

define LORA_A35_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 \
		$(@D)/mp257-lora \
		$(TARGET_DIR)/usr/bin/lora-A35
endef

$(eval $(generic-package))
