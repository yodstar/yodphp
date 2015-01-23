all: screw

screw: screw_encrypt.c
	gcc -o screw_encrypt screw_encrypt.c -lz

clean:
	/bin/rm screw_encrypt

install:
	cp -v screw_encrypt /usr/sbin
	cp -v screw_enc /usr/sbin
	chmod +x /usr/sbin/screw_encrypt
	chmod +x /usr/sbin/screw_enc
