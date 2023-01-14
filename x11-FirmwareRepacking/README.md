
# X11 Firmware Repacking
The code in this folder is used for unpacking the X11 BMC upgrade package, enabled SSH and repack it so that it can be uploaded to the server. 

This include two submodules: 
* `super-bmc-fw-tools` for encryption and decryption of the firmware, modified from https://github.com/c0d3z3r0/smcbmc 
* `ipmi_firmware_tools` for unpacking/repacking the decrypted firmware, modified from https://github.com/devicenull/ipmi_firmware_tools

use `git submodule update --init --recursive` to download the submodules.

You'll need to install cramfs support with `sudo apt-get install cramfsprogs fusecram` in order to use this tool. 

This tool has been tested with `SMT_X11_160.bin`. 

The firmware upgrade package can be downloaded from https://drunkencat.net/misc/SupermicroBIOS.html. 

## Usage
* Unpack firmware: `./fw_pack_unpacker.sh unpack ./SMT_X11_160.bin`
	* The firmware will be unpacked to `./data/` and the rootfs will be copied to `~/rootfs_modify/`
	* It also temporarily mount the rootfs to `~/temp_mount/`
* Enable SSH: `./fw_pack_unpacker.sh enablessh`
	* The rootfs will be modified to enable SSH access to BMC
* Repack firmware: `./fw_pack_unpacker.sh pack ./SMT_X11_160.bin`
	* This command will repack the modified rootfs and other images to a new firmware
	* The repacked firmware: `SMT_X11_163_rebuilt_encrypted.bin` can be uploaded through KCS/Web firmware upgrade of the BMC to enable SSH access to BMC. 
