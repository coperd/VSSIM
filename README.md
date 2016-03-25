### ttVSSIM: tail tolerate SSD Emulator based on QEMU ###

This project is modified from VSSIM (https://github.com/ESOS-Lab/VSSIM)

Changes:

  * multiple SSD support: integrate SSD Emulation to IDE
  * latency adding logic moved to AIO
  * EBUSY interface, return EIO directly for reads upon GC

