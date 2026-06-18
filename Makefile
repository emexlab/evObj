# Makefile

all: compile

# Prepares the build directory
build/.build:
	mkdir -p build
	cd build; \
		cmake ..
	touch build/.build

# Builds the targets
compile: build/.build
	cd build; \
		make

# And install
install: compile
	cd build; \
		sudo make install

clean:
	rm -rf build
