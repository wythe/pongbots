.PHONY: tags perf

all:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cd .. && make -C build -j12

debug:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && cd .. && make -C build -j12

serial:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make VERBOSE=1 && cd ..

clean:
	test -d build && make -C build clean
	test -d build && rm -rf build
