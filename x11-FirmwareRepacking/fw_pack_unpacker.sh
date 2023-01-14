#!/bin/bash

SMC_BMC_PATH="./super-bmc-fw-tools/"
IPMI_FWTOOL_PATH="./ipmi_firmware_tools/"

ACTION=$1
orgi_bin_path=$2

filename=$(basename -- "$orgi_bin_path")
nameonly="${filename%.bin}"

if [ "$ACTION" == "unpack" ]; then
	echo "unpacking firmware..."

	[[ -d ./data ]] || mkdir ./data

	# save decrypted fw to data folder
	decrypted_bin="./data/"$nameonly"_decrypted.bin"

	# decrypt firmware
	python3 $SMC_BMC_PATH"decryption_tool.py" $orgi_bin_path $decrypted_bin

	# For unencrypted fw
	# cp $orgi_bin_path $decrypted_bin

	# unpack firmware
	python2 $IPMI_FWTOOL_PATH"read_header.py" $decrypted_bin --extract
	
	# Extract the files in the image
	mkdir ~/temp_mount
	mkdir ~/rootfs_modify
	sudo mount -t cramfs -o loop ./data/out_rootfs_img.bin ~/temp_mount/
	sudo cp -a ~/temp_mount ~/rootfs_modify/
	sudo umount ~/temp_mount
	rmdir ~/temp_mount

	# the `footer_version` in data/image.ini is incorrectly detected, it should be changed to 3
	#echo "Fixing footer_version..."
	#sed -i 's/footer\_version\ \=\ 2/footer\_version\ \=\ 3/g' ./data/image.ini

	echo "unpacking done, the unpacked rootfs can be found in ~/rootfs_modify"
elif [ "$ACTION" == "pack" ]; then
	echo "packing firmware..."
	mv ./data/out_rootfs_img.bin ./data/out_rootfs_img.bin.orgi
	sudo mkfs.cramfs ~/rootfs_modify/temp_mount/ ./data/out_rootfs_img.bin

	# Repacking the image with ipmi fwtool
	$IPMI_FWTOOL_PATH"rebuild_image.py" 
	python3 $SMC_BMC_PATH"decryption_tool.py" -e ./data/rebuilt_image.bin ./data/$nameonly"_rebuilt_encrypted.bin"

	echo "modifying the magic value following ATENs and make it 0x3101 "
	# strings -atx ./data/SMT_X11_163_rebuilt_encrypted.bin | grep ATENs | awk '{print $1}'
	echo -ne "\x31\x01" | dd conv=notrunc of=./data/$nameonly"_rebuilt_encrypted.bin" bs=1 seek=$((0x01df6000 + 30)) count=2

	echo "packing done, the packed firmware can be found in ./data/"$nameonly"_rebuilt_encrypted.bin"
elif [ "$ACTION" == "enablessh" ]; then
	# overwrite the file with this simple sh script
	printf "#!/bin/sh\n/bin/sh\n" | sudo tee ~/rootfs_modify/temp_mount/SMASH/msh
else
	echo "Usage: $0 unpack|pack|enablessh <filename>"
	echo " 	example: $0 unpack ../firmware/SMT_X11_160/SMT_X11_160.bin"
	echo " 	example: $0 pack ../firmware/SMT_X11_160/SMT_X11_160.bin"
	echo " 	example: $0 enablessh"
	exit 1
fi



