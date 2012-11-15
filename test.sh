#!/bin/sh

usage()
{
cat << EOF
usage: $0 [options]
options: -s = swifter compile, otherwise default to v7s
         -h = help
EOF
}

INTERNAL_PLATFORM=



gcc -std=gnu99 -m32 -o race race.c -I. -F/System/Library/PrivateFrameworks -framework MobileDevice -framework CoreFoundation -Wall
gcc -std=gnu99 -m32 -o interact interact.c -I. -F/System/Library/PrivateFrameworks -framework MobileDevice -framework CoreFoundation -Wall

while getopts "sh" OPTION
do
	case $OPTION in
		h)
			usage
			exit 1
			;;
		s)
			INTERNAL_PLATFORM=swifter
			;;
	esac
done

echo Compiling for $INTERNAL_PLATFORM configuration
make PLATFORM=$INTERNAL_PLATFORM

cp obj/amfi_interpose.dylib ddi/
cp obj/jailbreak ddi/

pushd bs
tar c . > ../ddi/bootstrap.tar
popd

sudo chmod 0644 ddi/Library/LaunchDaemons/*.plist
sudo chown root:wheel ddi/Library/LaunchDaemons/*.plist

sudo chmod +x ddi/usr/libexec/*
sudo chown root:wheel ddi/usr/libexec/*

sudo rm ddi.dmg
sudo hdiutil create -format UDZO -layout NONE -srcfolder ddi ddi.dmg

for ((x=299999; x!=1000000; x+=1))
do
  OUTPUT=$(./race $x DeveloperDiskImage.dmg{,.signature} ddi.dmg)
  echo $OUTPUT

  if [[ $OUTPUT =~ Complete ]]; then
    echo "Success!"
    echo $x
    break
  fi

  echo $x 
done
#./race 300000 DeveloperDiskImage.dmg{,.signature} ddi.dmg


