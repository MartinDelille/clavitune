default: c b kill r

.PHONY: clean deps c b kill r

ifeq ($(shell uname),Darwin)
PROFILE ?= conan/profiles/macos
else
PROFILE ?= conan/profiles/linux
endif

all: clean_depts clean deps c b kill r

clean:
	rm -rf build compile_commands.json

clean_depts:
	uv run conan remove "stk/*"

deps:
	uv run conan install . --profile:all=$(PROFILE) --build=missing -s build_type=Release

c: deps
	cmake --preset conan-release
	ln -sf build/Release/compile_commands.json compile_commands.json

b:
	cmake --build --preset conan-release

# Conan's libalsa package doesn't ship the alsa-plugins ecosystem (pulse,
# pipewire, dmix, ...) or a working default plugin-dir baked into the
# library, so point it at the system's ALSA config/plugins at runtime.
r:
ifeq ($(shell uname),Linux)
	ALSA_CONFIG_PATH=/usr/share/alsa/alsa.conf \
	ALSA_PLUGIN_DIR=$(shell pkg-config --variable=libdir alsa 2>/dev/null || echo /usr/lib/x86_64-linux-gnu)/alsa-lib \
	./build/Release/clavitune
else
	./build/Release/clavitune
endif

kill:
	killall clavitune || exit 0
