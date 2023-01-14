# PMFault: Faulting and Bricking Server CPUs through Management Interfaces
**Disclaimer: The code in this repo can cause PERMANENT DAMAGE to your server. Use at your own risk.**

This repo contains the supplementary materials for the paper "PMFault: Faulting and Bricking Server CPUs through Management Interfaces", which will appear in [CHES 2023](https://ches.iacr.org/2023/). 

Check [our website](https://zt-chen.github.io/PMFault/) for a brief introduction of the PMFault attack.

A preprint of the paper is available at [here](./PMFault-V1.4-preprint.pdf).

## Folder Structure
### Attack to Supermicro X11 Motherboards

* [x11-undervolt_from_rpi](./x11-undervolt_from_rpi.c): PoC of undervolting via raspberry pi, used in initial investigation
* [x11-FirmwareRepacking](./x11-FirmwareRepacking/): PoC of firmware repacking to enable SSH access to BMC
* [x11-BMC-DMA-PMBus](./x11-BMC-DMA-PMBus): PoC of PMBus voltage control via code execution on BMC
	* brick.c : overvolt and brick the CPU
	* undervolt.c : undervolt and inject fault to SGX
* [x11-voltage-change-with-ipmitool](./x11-voltage-change-with-ipmitool.md): Steps for changeing the voltage via ipmitool (over lan/kcs)

The target of the fault injection is the same as those used in [PlunderVolt](https://github.com/KitMurdock/plundervolt)
### Attack to ASRock Rack Motherboard
* asrock-pmbus-powerdown: PoC of powerdown via PMBus for ASRock motherboard, execute with root privilege on CPU

### PMBusDetect Tool
* [pmbusdetect](./pmbusdetect): PMBusDetect tool for detecting the connection between CPU/BMC and VRM. 
	* currently it support ISL68137 and MP2955, welcome to add support for other VRM by opening an issue or pull request.

## Library
If you are using the OS provided i2c bus (`/dev/i2c-X`) to communicate with PMBus, you'll need to:

1. Load the kernel module to enable i2c bus, different motherboard may need different kernel module, check which one to use at [here](https://docs.kernel.org/i2c/busses/index.html) `sudo modprobe i2c-i801` works for Supermicro X11. 
2. `libi2c` library is required for building `PMBusDetect`, `asrock-pmbus-powerdown` and `x11-undervolt_from_rpi`, you can install it with `sudo apt-get install -y libi2c-dev` or compile from source: [i2c-tools](https://github.com/mozilla-b2g/i2c-tools)

