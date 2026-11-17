FILE ?= build/bootupdate-kmx_v3.bin
OFFSET ?= 0x4000

.PHONY: all kflash flash clean
all:
ifeq ($(wildcard build/CMakeCache.txt),)
	cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
endif
	cmake --build build

kflash: all
	cmake --build build --target kflash

# PORT is the UART name, for example: make flash PORT=COM5
flash:
ifeq ($(strip $(PORT)),)
	$(error Set PORT to the UART port (for example PORT=COM5))
endif
	$(MAKE) all
	kdiag module flash --file "$(FILE)" --offset "$(OFFSET)" --port "uart:$(PORT)" --reset

clean:
	cmake -E rm -rf build
