#!/bin/bash

# VSSIM running script


IMAGEDIR=$HOME/images
RDSKDIR=/mnt/tmpfs

#VSSIMQEMU=$HOME/git/VSSIM/QEMU/x86_64-softmmu/qemu-system-x86_64.noebusy
VSSIMQEMU=./qemu-system-x86_64
VSSIMQEMUIMG=../qemu-img


LDSK=$IMAGEDIR/u14s_old.raw # virtual disk for guest OS, reside in host local FS
#LDSK=$IMAGEDIR/ssd_hda.img # virtual disk for guest OS, reside in host local FS

VSSD1=$RDSKDIR/vssd1.raw   # virtual SSD disks (1,2) for building RAID1 in guest OS, reside in host ramdisk
VSSD2=$RDSKDIR/vssd2.raw
VSSD3=$RDSKDIR/vssd3.raw
VSSD4=$RDSKDIR/vssd4.raw


# check if tmpfs has been mounted, if not, mount it now
ISRDSK=$(mount | grep -i "$RDSKDIR")
if [[ $ISRDSK == "" ]]; then
    sudo mkdir -p $RDSKDIR
    sudo mount -t tmpfs -o size=24G tmpfs $RDSKDIR
fi

if [[ "X""$1" == "X" ]]; then
    QEMU=$VSSIMQEMU
else
    QEMU=$1
fi
# create virtual disks we need if they doesn't exist
[[ ! -e $VSSD1 ]] && $VSSIMQEMUIMG create -f raw $VSSD1 6G
[[ ! -e $VSSD2 ]] && $VSSIMQEMUIMG create -f raw $VSSD2 6G
[[ ! -e $VSSD3 ]] && $VSSIMQEMUIMG create -f raw $VSSD3 6G
[[ ! -e $VSSD4 ]] && $VSSIMQEMUIMG create -f raw $VSSD4 6G



# uncomment this part when you setup new environments
#rm -rf $VSSD1 $VSSD2
#$VSSIMQEMUIMG create -f raw $VSSD1 1G
#$VSSIMQEMUIMG create -f raw $VSSD2 1G


# start the VM
# -s -S for guest kernel debugging
# $VSSD1 -- guest sda
# $VSSD2 -- guest sdb
# $VSSD3 -- guest sdc
    #-drive file=$VSSD2,if=ide \
    #-drive file=$VSSD3,if=ide \
    #-drive file=$VSSD4,if=ide \

$QEMU \
    -name "VSSIM" \
    -smp 2 \
    -m 2048 \
    -drive file=$LDSK,if=virtio,boot=on \
    -drive file=$VSSD1,if=ide \
    -drive file=$VSSD2,if=ide \
    -drive file=$VSSD3,if=ide \
    -drive file=$VSSD4,if=ide \
    -enable-kvm \
    -net nic,model=virtio \
    -net user,hostfwd=tcp::8080-:22 \
    -nographic \
    -usbdevice tablet
