#!/bin/bash

# VSSIM running script

ME=$(who am i | awk '{print $1}') 

IMAGEDIR=$HOME/images
RDSKDIR=/mnt/tmpfs

#VSSIMQEMU=$HOME/git/VSSIM/QEMU/x86_64-softmmu/qemu-system-x86_64.noebusy
VSSIMQEMU=./qemu-system-x86_64
VSSIMQEMUIMG=../qemu-img


LDSK=$IMAGEDIR/u14s_old.raw # virtual disk for guest OS, reside in host local FS
#LDSK=$IMAGEDIR/ssd_hda.img # virtual disk for guest OS, reside in host local FS

# check if tmpfs has been mounted, if not, mount it now
ISRDSK=$(mount | grep -i "$RDSKDIR")
if [[ "X"$ISRDSK == "X" ]]; then
    sudo mkdir -p $RDSKDIR
    sudo mount -t tmpfs -o size=24G tmpfs $RDSKDIR
    sudo chown -R $ME:$ME /mnt
fi

if [[ "X""$1" == "X" ]]; then
    QEMU=$VSSIMQEMU
else
    QEMU=$1
fi

# create virtual disks we need if they don't exist
for i in $(seq 4); do
    vssd=$RDSKDIR/vssd${i}.raw
    [[ ! -e $vssd ]] && $VSSIMQEMUIMG create -f raw $vssd 6G
done

# start the VM
# -s -S for guest kernel debugging


$QEMU \
    -name "VSSIM" \
    -smp 2 \
    -m 4096 \
    -drive file=$LDSK,if=virtio,boot=on \
    -drive file=$RDSKDIR/vssd1.raw,if=ide \
    -drive file=$RDSKDIR/vssd2.raw,if=ide \
    -drive file=$RDSKDIR/vssd3.raw,if=ide \
    -drive file=$RDSKDIR/vssd4.raw,if=ide \
    -enable-kvm \
    -net nic,model=virtio \
    -net user,hostfwd=tcp::8080-:22 \
    -nographic \
    -usbdevice tablet
