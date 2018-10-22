.PHONY: configure compile

BUILD = build
MKFLAGS = -j

compile: configure
	make -s -C $(BUILD) $(MKFLAGS)

configure:
	mkdir -p $(BUILD)
	cd $(BUILD) ; cmake ..
