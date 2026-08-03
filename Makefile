default: c b kill r

.PHONY: clean deps c b kill r

PROFILE ?= conan/profiles/macos

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

r:
	./build/Release/clavitune

kill:
	killall clavitune || exit 0
