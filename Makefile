all: service stop install start

service:
	$(MAKE) -C src

stop:
	novacom run file:///sbin/stop org.webosinternals.keyboss

install:
	novacom put file:///var/usr/sbin/org.webosinternals.keyboss < src/keyboss

start:
	novacom run file:///sbin/start org.webosinternals.keyboss

clobber:
	$(MAKE) -C src clobber
