# SPDX-License-Identifier: GPL-2.0
all: test mod
test: virtio_test vringh_test
virtio_test: virtio_ring.o virtio_test.o
vringh_test: vringh_test.o vringh.o virtio_ring.o

CFLAGS += -g -O2 -Werror -Wall -I. -I../include/ -I ../../usr/include/ -Wno-pointer-sign -fno-strict-overflow -fno-strict-aliasing -fno-common -MMD -U_FORTIFY_SOURCE
vpath %.c ../../drivers/virtio ../../drivers/vhost
mod:
	${MAKE} -C `pwd`/../.. M=`pwd`/vhost_test V=${V}

#oot: build vhost as an out of tree module for a distro kernel
#no effort is taken to make it actually build or work, but tends to mostly work
#if the distro kernel is very close to upstream
#unsupported! this is a development tool only, don't use the
#resulting modules in production!
KSRC=/lib/modules/$$(uname -r)/build
oot:
	echo "UNSUPPORTED! Don't use the resulting modules in production!"
	KCFLAGS="-I "`pwd`/../../drivers/vhost ${MAKE} -C ${KSRC} M=`pwd`/vhost_test V=${V}
	KCFLAGS="-I "`pwd`/../../drivers/vhost ${MAKE} -C ${KSRC} M=`pwd`/../../drivers/vhost V=${V} CONFIG_VHOST_VSOCK=n

.PHONY: all test mod clean vhost oot
clean:
	${RM} *.o vringh_test virtio_test vhost_test/*.o vhost_test/.*.cmd \
              vhost_test/Module.symvers vhost_test/modules.order *.d
-include *.d
