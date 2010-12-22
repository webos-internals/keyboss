APP_ID=$(shell grep id appinfo.json | cut -d\" -f4)
VERSION=$(shell grep version appinfo.json | cut -d\" -f4)
META_VERSION=1

default: doit-tcp

doit: package-arm palm-install palm-launch

doit-tcp: package-i686 palm-install-tcp palm-launch-tcp

mojo: mojo-tcp install-tcp launch-tcp

mojo-tcp:
	palm-package .

install-tcp:
	palm-install -d tcp ${APP_ID}_${VERSION}_all.ipk

launch-tcp:
	palm-launch -d tcp ${APP_ID}

package-arm: ipkgs/${APP_ID}_${VERSION}-${META_VERSION}_arm.ipk
package-i686: ipkgs/${APP_ID}_${VERSION}-${META_VERSION}_i686.ipk

palm-launch:
	palm-launch ${APP_ID}

palm-install:
	palm-install ipkgs/${APP_ID}_${VERSION}-${META_VERSION}_arm.ipk

palm-launch-tcp:
	palm-launch -d tcp ${APP_ID}

palm-install-tcp:
	palm-install -d tcp ipkgs/${APP_ID}_${VERSION}-${META_VERSION}_i686.ipk

ipkgs/${APP_ID}_${VERSION}-${META_VERSION}_i686.ipk: service build/i686/CONTROL/control
	cp control/* build/i686/CONTROL/
	mkdir -p build/i686/usr/palm/applications/${APP_ID}
	cp -r app build/i686/usr/palm/applications/${APP_ID}
	cp *.json build/i686/usr/palm/applications/${APP_ID}
	cp *.html build/i686/usr/palm/applications/${APP_ID}
	cp *.png build/i686/usr/palm/applications/${APP_ID}
	cp -r images build/i686/usr/palm/applications/${APP_ID}
	cp -r stylesheets build/i686/usr/palm/applications/${APP_ID}
	cp -r udev build/i686/usr/palm/applications/${APP_ID}
	cp -r dbus build/i686/usr/palm/applications/${APP_ID}
	mkdir -p build/i686/usr/palm/applications/${APP_ID}/bin
	install -m 755 src/keyboss build/i686/usr/palm/applications/${APP_ID}/bin/${APP_ID}
	mkdir -p ipkgs
	( cd build; TAR_OPTIONS="--wildcards --mode=g-s" ipkg-build -o 0 -g 0 -p i686 )
	mv build/${APP_ID}_${VERSION}-${META_VERSION}_i686.ipk $@
	ipkg-make-index -v -p ipkgs/Packages ipkgs
	#rsync -av ipkgs/ /source/www/feeds/test

ipkgs/${APP_ID}_${VERSION}-${META_VERSION}_arm.ipk: service build/arm/CONTROL/control
	cp control/* build/arm/CONTROL/
	mkdir -p build/arm/usr/palm/applications/${APP_ID}
	cp -r app build/arm/usr/palm/applications/${APP_ID}
	cp *.json build/arm/usr/palm/applications/${APP_ID}
	cp *.html build/arm/usr/palm/applications/${APP_ID}
	cp *.png build/arm/usr/palm/applications/${APP_ID}
	cp -r images build/arm/usr/palm/applications/${APP_ID}
	cp -r stylesheets build/arm/usr/palm/applications/${APP_ID}
	#cp -r upstart build/arm/usr/palm/applications/${APP_ID}
	cp -r udev build/arm/usr/palm/applications/${APP_ID}
	cp -r dbus build/arm/usr/palm/applications/${APP_ID}
	mkdir -p build/arm/usr/palm/applications/${APP_ID}/bin
	install -m 755 src/keyboss build/arm/usr/palm/applications/${APP_ID}/bin/${APP_ID}
	mkdir -p ipkgs
	( cd build; TAR_OPTIONS="--wildcards --mode=g-s" ipkg-build -o 0 -g 0 -p arm )
	mv build/${APP_ID}_${VERSION}-${META_VERSION}_arm.ipk $@
	ipkg-make-index -v -p ipkgs/Packages ipkgs
	#rsync -av ipkgs/ /source/www/feeds/test
	
build/%/CONTROL/control:
	mkdir -p build/$*/CONTROL
	rm -f $@
	@echo "Package: ${APP_ID}" > $@
	@echo "Version: ${VERSION}-${META_VERSION}" >> $@
	@echo "Architecture: $*" >> $@
	@echo "Maintainer: Eric Gaudet <emoney_33@yahoo.com>" >> $@
	@echo "Description: KeyBoss" >> $@
	@echo "Section: System Utilities" >> $@
	@echo "Priority: optional" >> $@
	@echo "Source: {}" >> $@

src:
	rm -rf build ipkgs
	
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
	@rm -rf build ipkgs
