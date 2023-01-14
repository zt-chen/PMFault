
# Voltage Changing via IPMI 

Global Variables:

```bash
BUSNUM=$((0x3))
VRMADDRW=$((0x20))
VRMADDRR=$(($VRMADDRW<<1 | 1))
```

## Step1: Read OPERATION and MFR_VR_CONFIG 
```bash
ipmitool i2c bus=$BUSNUM $VRMADDRR 0x01 0x01			# Read OPERATION, 1byte
ipmitool i2c bus=$BUSNUM $VRMADDRR 0x02 0xe4			# Read MFR_VR_CONFIG, 2byte 
```
Note: the 2bytes responsed of MFR_VR_CONFIG is lower byte first

## Step2: Set VOUT_CMD to 0 
Setting this regisiter to 0xffff will overvolt and destory the CPU, we set it 0x0000 here for testing and crash the system
```bash
ipmitool i2c bus=$BUSNUM $VRMADDRW 0x00 0x21 0x00 0x00 # Write 0x0000 to VOUT_CMD and read 0 bytes, 
```

## Step3: Enable overclocking mode 
### 3.1: Set OPERATIONS to enable PMBus override
```bash
<OPERATION REG value> = <the OPERATION REG read from Step 1> | 0x20
echo "Configuring OPERATION"
ipmitool i2c bus=$BUSNUM $VRMADDRW 0x00 0x01 <OPERATION REG value> 	
```

### 3.2 Set MFR_VR_CONFIG to enable fixed overclocking mode
`<lower byte> = <lower byte from step 1> | 0x8`
IMPORTANT: Also make sure bit 0 of the higher byte is 1, otherwith it is using 10mV SVID table which can DESTORY the CPU!!!!!

```bash
echo "Configuring MFR_VR_CONFIG"
ipmitool i2c bus=$BUSNUM $VRMADDRW 0x00 0xe4 <lower byte> <higher byte>
```
