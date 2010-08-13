APP_ID=$(shell grep id appinfo.json | cut -d\" -f4)
VERSION=$(shell grep version appinfo.json | cut -d\" -f4)

doit: test palm-install

test: ipkgs/${APP_ID}_${VERSION}_arm.ipk

palm-install:
	#pdk-install ipkgs/${APP_ID}_${VERSION}_arm.ipk
	palm-install ipkgs/${APP_ID}_${VERSION}_arm.ipk

ipkgs/${APP_ID}_${VERSION}_arm.ipk: service build/arm/CONTROL/control
	cp control/* build/arm/CONTROL/
	mkdir -p build/arm/usr/palm/applications/${APP_ID}
	cp -r app build/arm/usr/palm/applications/${APP_ID}
	cp *.json build/arm/usr/palm/applications/${APP_ID}
	cp *.html build/arm/usr/palm/applications/${APP_ID}
	cp *.png build/arm/usr/palm/applications/${APP_ID}
	#cp -r images build/arm/usr/palm/applications/${APP_ID}
	cp -r stylesheets build/arm/usr/palm/applications/${APP_ID}
	cp -r upstart build/arm/usr/palm/applications/${APP_ID}
	cp -r udev build/arm/usr/palm/applications/${APP_ID}
	mkdir -p build/arm/usr/palm/applications/${APP_ID}/bin
	install -m 755 src/keyboss build/arm/usr/palm/applications/${APP_ID}/bin/${APP_ID}
	mkdir -p ipkgs
	( cd build; TAR_OPTIONS="--wildcards --mode=g-s" ipkg-build -o 0 -g 0 -p arm )
	mv build/${APP_ID}_${VERSION}_arm.ipk $@
	ipkg-make-index -v -p ipkgs/Packages ipkgs
	#rsync -av ipkgs/ /source/www/feeds/test
	
build/%/CONTROL/control:
	mkdir -p build/arm/CONTROL
	rm -f $@
	@echo "Package: ${APP_ID}" > $@
	@echo "Version: ${VERSION}" >> $@
	@echo "Architecture: arm" >> $@
	@echo "Maintainer: Eric J Gaudet <emoney_33@yahoo.com>" >> $@
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
