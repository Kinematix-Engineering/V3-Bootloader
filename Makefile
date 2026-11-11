.PHONY: all clean
all:
	@test -f build/CMakeCache.txt || cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
	cmake --build build

clean:
	rm -rf build
