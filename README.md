# Cryptodev 

The Virtual Crypto Device allows processes inside the Virtual Machine to access the real cryptographic device of the host machine, type cryptodev-linux, like hardware crypto accelerators, with the technique of paravirtualization.

The application developed an encrypted chat over TCP/IP sockets using BSD Sockets API.

---

Crypto Device VirtIO is developed to access the crypto accelerators and the responsible driver for the guest kernel Linux inside the QEMU virtual machine.

The following architecture depicts a frontend inside the VM and backend as a part of QEMU using the communication of VirtIO.

For the VM, the implementation of ioctl() functions as the driver of cryptodev. While for the qemu, the calls of the frontend are served and pushed towards processing from the hardware.

![](/images/arch.png)


