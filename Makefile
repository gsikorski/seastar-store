COMPILER = g++
CFLAGS = -g -O3
MODE = release

SRC = $(wildcard *.cc)
OBJ = $(SRC:.cc=.o)

.cc.o:
	$(COMPILER) -c -o $@ $< $(shell pkg-config --cflags --static /opt/seastar/build/$(MODE)/seastar.pc) $(CFLAGS)

app: /opt/seastar/build/$(MODE)/libseastar.a $(OBJ)
	$(COMPILER) $(SRC:.cc=.o) $(shell pkg-config --libs --cflags --static /opt/seastar/build/$(MODE)/seastar.pc) $(CFLAGS) -o app

/opt/seastar/build/$(MODE)/libseastar.a:
	cd /opt/seastar && ./configure.py --mode="$(MODE)" --disable-dpdk --disable-hwloc --cflags="$(CFLAGS)" --compiler="$(COMPILER)"
	ninja -C /opt/seastar/build/$(MODE) libseastar.a

$(OBJ): $(filter-out app.h,$(SRC:.cc=.h))

clean:
	rm -f *~ *.o app