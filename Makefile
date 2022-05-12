COMPILER = g++
CFLAGS = -g
MODE = release

app: seastar/build/release/libseastar.a app.cc
	$(COMPILER) app.cc $(shell pkg-config --libs --cflags --static seastar/build/$(MODE)/seastar.pc) $(CFLAGS) -o app

seastar/build/release/libseastar.a:
	cd ./seastar && ./configure.py --mode="$(MODE)" --disable-dpdk --disable-hwloc --cflags="$(CFLAGS)" --compiler="$(COMPILER)"
	ninja -C seastar/build/$(MODE) libseastar.a
