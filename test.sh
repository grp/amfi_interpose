#!/bin/sh

gcc -std=gnu99 -m32 -o race race.c -I. -F/System/Library/PrivateFrameworks -framework MobileDevice -framework CoreFoundation -Wall
gcc -std=gnu99 -m32 -o interact interact.c -I. -F/System/Library/PrivateFrameworks -framework MobileDevice -framework CoreFoundation -Wall

make

cp obj/amfi_interpose.dylib ddi/
cp obj/jailbreak ddi/

sudo chmod 0644 ddi/Library/LaunchDaemons/*.plist
sudo chown root:wheel ddi/Library/LaunchDaemons/*.plist

sudo chmod +x ddi/usr/libexec/*
sudo chown root:wheel ddi/usr/libexec/*

rm ddi.dmg
hdiutil create -format UDZO -layout NONE -srcfolder ddi ddi.dmg

race 100000 DeveloperDiskImage.dmg{,.signature} ddi.dmg


