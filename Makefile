default: c b kill r

.PHONY: clean c b kill r

all: clean c b kill r

clean:
	rm -rf build compile_commands.json

c:
	cmake -S . -B build
	ln -sf build/compile_commands.json compile_commands.json

b:
	cmake --build build

r:
	./build/clavitune

kill:
	killall clavitune || exit 0
