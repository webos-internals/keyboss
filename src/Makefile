INCLUDES=-I/source/webos-internals/lunaservice -I. -I${ROOT}${prefix}/include/glib-2.0 -I${ROOT}${prefix}/lib/glib-2.0/include -I${ROOT}${prefix}/include
LDFLAGS=-L. -L${ROOT}${prefix}/glib-2.0 -L${ROOT}${prefix}/lib -L/source/webos-internals/lunaservice -Wl,--allow-shlib-undefined
LIBS=-llunaservice -lglib-2.0 -lpthread -lmjson

OBJECTS=keyboss.o luna_service.o luna_methods.o

all: keyboss

$(OBJECTS):
	$(CC) -o $@ -c ${@:.o=.c} $(INCLUDES)

keyboss: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

clean:
	rm -f keyboss $(OBJECTS)
