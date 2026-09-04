.PHONY: build clean rebuild

build:
	cmake -B build
	cmake --build build

clean:
	rm -rf build

rebuild: clean build

