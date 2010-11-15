#?CC=gcc
#?CFLAGS=-Wall -Os -march=i586
#?LDFLAGS=-static
#?all:	ns nsnetif nsrootfs veth nscgdev macvlan
#?ns:	ns.o jelist.o jelopt.o cmd.o caps.o anonsocket.o
#?	$(CC) $(LDFLAGS) -o ns ns.o jelist.o  jelopt.o cmd.o caps.o anonsocket.o
#?nsnetif:	nsnetif.o lxc_device_move.o nl.o lxc_device_rename.o jelist.o jelopt.o
#?	$(CC) $(LDFLAGS) -o nsnetif nsnetif.o lxc_device_move.o nl.o lxc_device_rename.o jelist.o jelopt.o
#?nscgdev:	nscgdev.o cgroup.o jelist.o jelopt.o
#?	$(CC) $(LDFLAGS) -o nscgdev nscgdev.o cgroup.o jelist.o jelopt.o
#?macvlan:	macvlan.o lxc_macvlan_create.o nl.o jelist.o jelopt.o
#?	$(CC) $(LDFLAGS) -o macvlan macvlan.o lxc_macvlan_create.o nl.o jelist.o jelopt.o
#?veth:	veth.o lxc_veth_create.o nl.o jelist.o jelopt.o
#?	$(CC) $(LDFLAGS) -o veth veth.o lxc_veth_create.o nl.o jelist.o jelopt.o
#?cmdtest:	cmdtest.o cmd.o
#?	$(CC) $(LDFLAGS) -o cmdtest cmdtest.o cmd.o
#?clean:
#?	rm -f *.o ns nsnetif veth nsrootfs nscgdev cmdtest macvlan
#?install:	all
#?	mkdir -p $(DESTDIR)/sbin
#?	cp -f ns nsnetif veth nsrootfs nscgdev macvlan $(DESTDIR)/sbin
#?	chown 0 $(DESTDIR)/sbin/ns $(DESTDIR)/sbin/nsnetif $(DESTDIR)/sbin/veth $(DESTDIR)/sbin/nscgdev $(DESTDIR)/sbin/macvlan
#?	chmod +s $(DESTDIR)/sbin/ns $(DESTDIR)/sbin/nsnetif $(DESTDIR)/sbin/veth $(DESTDIR)/sbin/nscgdev $(DESTDIR)/sbin/macvlan
#?tarball:	clean
#?	make-tarball.sh
CC=gcc
CFLAGS=-Wall -Os -march=i586
LDFLAGS=-static
all:	ns nsnetif nsrootfs veth nscgdev macvlan
ns:	ns.o jelist.o jelopt.o cmd.o caps.o anonsocket.o
	$(CC) $(LDFLAGS) -o ns ns.o jelist.o  jelopt.o cmd.o caps.o anonsocket.o
nsnetif:	nsnetif.o lxc_device_move.o nl.o lxc_device_rename.o jelist.o jelopt.o
	$(CC) $(LDFLAGS) -o nsnetif nsnetif.o lxc_device_move.o nl.o lxc_device_rename.o jelist.o jelopt.o
nscgdev:	nscgdev.o cgroup.o jelist.o jelopt.o
	$(CC) $(LDFLAGS) -o nscgdev nscgdev.o cgroup.o jelist.o jelopt.o
macvlan:	macvlan.o lxc_macvlan_create.o nl.o jelist.o jelopt.o
	$(CC) $(LDFLAGS) -o macvlan macvlan.o lxc_macvlan_create.o nl.o jelist.o jelopt.o
veth:	veth.o lxc_veth_create.o nl.o jelist.o jelopt.o
	$(CC) $(LDFLAGS) -o veth veth.o lxc_veth_create.o nl.o jelist.o jelopt.o
cmdtest:	cmdtest.o cmd.o
	$(CC) $(LDFLAGS) -o cmdtest cmdtest.o cmd.o
clean:
	rm -f *.o ns nsnetif veth nsrootfs nscgdev cmdtest macvlan
install:	all
	mkdir -p $(DESTDIR)/sbin
	cp -f ns nsnetif veth nsrootfs nscgdev macvlan $(DESTDIR)/sbin
	chown 0 $(DESTDIR)/sbin/ns $(DESTDIR)/sbin/nsnetif $(DESTDIR)/sbin/veth $(DESTDIR)/sbin/nscgdev $(DESTDIR)/sbin/macvlan
	chmod +s $(DESTDIR)/sbin/ns $(DESTDIR)/sbin/nsnetif $(DESTDIR)/sbin/veth $(DESTDIR)/sbin/nscgdev $(DESTDIR)/sbin/macvlan
tarball:	clean
	make-tarball.sh
