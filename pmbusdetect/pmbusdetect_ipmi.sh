#!/bin/bash
# Note: All bit count starts with 0


# CHANGE these variables
BUSNUM=1
DEBUG=0

IPMI_LAN_FLAG=1
IPMI_LAN_HOST=192.168.8.33
IPMI_LAN_USER=admin
IPMI_LAN_PASS=admin

# This should be set to 1 to indicate read, however there is a bug in ASRock implementation
ADDR_CALC_SET_RBIT=0

# Function for reading pmbus command
i2c_read() {
    I2C_OUTPUT=""
    I2C_BYTES_READ=""
    I2C_BITS_READ=""
    local i2c_bus=$1
    local i2c_dev_addr=$2
    local i2c_read_bytes_count=$3
    local i2c_send_data="${@:4}"
    local i2c_addr_r

    if [ $ADDR_CALC_SET_RBIT -eq 1 ]; then
        # On Supermicro
        # The last bit of the addr indicates read or write
        # Setting 1 to indicate read
        i2c_addr_r=$(($i2c_dev_addr<<1 | 0x01))
    else
        # ASRock detect the R/W by TODO
        i2c_addr_r=$(($i2c_dev_addr<<1))
    fi

    local read_output

    if [[ $IPMI_LAN_FLAG -gt 0 ]]; then
          if [[ $DEBUG -gt 0 ]]; then
            echo "ipmitool i2c bus=$i2c_bus chan=0 $i2c_addr_r $i2c_read_bytes_count $i2c_send_data"
        fi
        # ipmitool -I lan -H $IPMI_LAN_HOST -U $IPMI_LAN_USER -P $IPMI_LAN_PASS i2c bus=$i2c_bus chan=0 $i2c_addr_r $i2c_read_bytes_count $i2c_send_data
        read_output=$(ipmitool -I lan -H $IPMI_LAN_HOST -U $IPMI_LAN_USER -P $IPMI_LAN_PASS i2c bus=$i2c_bus chan=0 $i2c_addr_r $i2c_read_bytes_count $i2c_send_data 2>/dev/null)
        res=$?
    else
        if [[ $DEBUG -gt 0 ]]; then
            echo "sudo ipmitool i2c bus=$i2c_bus chan=0 $i2c_addr_r $i2c_read_bytes_count $i2c_send_data"
        fi
        read_output=$(sudo ipmitool i2c bus=$i2c_bus chan=0 $i2c_addr_r $i2c_read_bytes_count $i2c_send_data 2>/dev/null)
        res=$?
    fi
    I2C_OUTPUT=$(echo -n $read_output | cut -f 1)
    I2C_BYTES_READ=$(echo -n $I2C_OUTPUT | cut -d " " -f 1,$i2c_read_bytes_count | tr -d '\n')         # The bytes part of the output
    I2C_BITS_READ=$(echo -n $I2C_OUTPUT | cut -d " " -f 1,$i2c_read_bytes_count | tr -d '\n')         # The bytes part of the output 
    return $res
}

pmbus_check_read() {
    local busnum=$1
    local dev_addr=$2
    local bytes_read=$3
    local cmd_name=$4
    local cmd_value=$5

    i2c_read $busnum $dev_addr $bytes_read $cmd_value
    local read_res=$?
    if [[ ! -z "$I2C_OUTPUT" || $read_res -eq 0 ]]; then
        printf 'Bus %d, \t Device 0x%x, \t %s success, data: ' $busnum $dev_addr $cmd_name
        echo $I2C_BYTES_READ
        local res=0
    fi 
    return $res
}


echo "Scanning all buses ..."
# Search all 16 buses
for busnum in {1..16}; do
    for dev_addr in {16..127}; do 
        vrm_found=0

        # dev_addr=0x60
        if [[ $DEBUG -gt 0 ]]; then
            echo "Bus $busnum, addr $dev_addr"
        fi

        # busnum=11
        # echo "Bus $busnum addr: 0x60"
        # BUSNUM=$b
        # When the address of the VR is 0x20
        # VRMADDRW=0x60 #$i
        # VRMADDRR=$(($VRMADDRW<<1 | 1))
        # VRMADDRR=0xC0
        
        # read_output=$(i2c_read $busnum $dev_addr 2 0x21)
        i2c_read $busnum $dev_addr 2 0x8d       # READ_TEMPERATURE
        read_res=$?
        if [[ ! -z "$I2C_OUTPUT" || $read_res -eq 0 ]]; then
            if [[ "$I2C_BYTES_READ" != "ff ff" && "$I2C_BYTES_READ" != "ff 00" && "$I2C_BYTES_READ" != "00 00" && "$I2C_BYTES_READ" != "00 ff" ]]; then
                echo ""
                printf "!!!!!!!!!!! Detected! Device addr: 0x%x at Bus: %d !!!!!!!!!!!\n" $dev_addr $busnum
                printf 'Bus %d, \t Device 0x%x, \t READ_TEMPERATURE success, data: ' $busnum $dev_addr
                echo $I2C_BYTES_READ

                vrm_found=1
            fi
        fi

        # If VRM is found, read the VRM informations
        if [[ $vrm_found -ne 0 ]]; then

            ###### Detect Vendor #####
	        # Check if the device responds to ISL_DEVICE_ID (ISL68137)
            pmbus_check_read $busnum $dev_addr 4 "ISL_DEVICE_ID" 0xad
            if [[ $? -eq 0 ]]; then
                printf "This device is likely to be an ISL VRM\n"
            fi
            # Check if the device responds to SVID_VENDOR_PRODUCT_ID (0xBF)
            pmbus_check_read $busnum $dev_addr 4 "SVID_VENDOR_PRODUCT_ID" 0xbf
            if [[ $? -eq 0 ]]; then
                printf "This device is likely to be a MPS VRM\n"
            fi

            ###### Read other info #####
            # Read the PMBus version
            pmbus_check_read $busnum $dev_addr 1 "PMBUS_VERSION" 0x98
            # READ_VOUT
            pmbus_check_read $busnum $dev_addr 2 "READ_VOUT" 0x8b
            # Read MFR_ID
            pmbus_check_read $busnum $dev_addr 16 "MFR_ID" 0x99

        fi
    done
done