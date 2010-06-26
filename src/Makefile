STAGING_DIR=${ROOT}

VERSION = unknown
CFLAGS := -g -DVERSION=\"${VERSION}\" \
					-I${STAGING_DIR}/usr/include/glib-2.0 \
					-I${STAGING_DIR}/usr/lib/glib-2.0/include \
					-I${STAGING_DIR}/usr/include \
					-I.
LDFLAGS := -g -L${STAGING_DIR}/usr/lib -llunaservice -lmjson -lglib-2.0 -lpthread

OBJECTS=keyboss.o luna_service.o luna_methods.o

all: keyboss

keyboss: $(OBJECTS)

clobber:
	rm -f keyboss $(OBJECTS)
