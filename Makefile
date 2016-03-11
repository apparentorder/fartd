all: fartd

fartd: fartd.c
	cc -o fartd -lgpio fartd.c

clean:
	rm -f fartd

install: fartd fartd.rc
	install -C -m 0755 fartd    /usr/local/sbin/fartd
	install -C -m 0755 fartd.rc /usr/local/etc/rc.d/fartd
	@echo "Set fartd_enable=yes in rc.conf to enable auto-start, or run 'service fartd onestart' to start it just once."

