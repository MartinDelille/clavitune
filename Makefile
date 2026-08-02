default: c b r

.PHONY: clean c b r

all: clean c b r

clean:
	rm -rf build compile_commands.json

c:
	cmake -S . -B build
	ln -sf build/compile_commands.json compile_commands.json

b:
	cmake --build build

r:
	./build/clavitune
