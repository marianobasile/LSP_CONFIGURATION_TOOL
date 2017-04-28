#!/bin/bash
startQuagga() {

echo ""
echo -e "========== Welcome to Quagga Configuration Tool! =========="

validOctet=0
while true
do   
echo ""
echo -en "Enter eth0 ip address: " 
read ip_address 
if [[ $ip_address =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]
then
ip_addr=( $(echo $ip_address | awk '{split($0,octet,"."); print octet[1],octet[2],octet[3],octet[4]}') )
for (( i=0; i<${#ip_addr[@]}; i++ ))
do
if [[ ${ip_addr[$i]} -lt 0 || ${ip_addr[$i]} -gt 255 ]]
then
echo -e "\nInvalid ip address!!"
break
else
validOctet=$((validOctet+1))
fi
done
else
echo -e "\nInvalid ip address!!"
fi
if [ $validOctet -eq 4 ]; then
break
fi
done

netPrefix=24

while true
do
echo ""
echo -en "Enter eth0 netmask: "
read netmask
if [[ $netmask =~ ^[0-9]{1,2}$ ]]
then
if [ $netmask -ne $netPrefix ]
then
echo -e "\nInvalid netmask!"
else
break
fi
else
echo -e "\nInvalid netmask!"
fi
done   
     
vtysh -c 'configure terminal' -c 'interface eth0' -c 'ip address '$ip_address'/'$netmask'' -c 'link-detect' -c 'exit' -c 'router ospf' -c 'network '${ip_addr[0]}'.'${ip_addr[1]}'.'${ip_addr[2]}'.'0'/'$netmask' area 1'
echo ""
vtysh -c 'write'

echo ""
echo -e "========== QUAGGA CONFIGURATION COMPLETED!! =========="
echo ""

echo -en "Running OSPF..."
sleep $1
echo "Done!"

./get_topology.sh
}

if [ "$(id -u)" != "0" ]; then 
echo ""
echo "Run script as root: sudo $0" 
exit 1 
fi

cp /usr/local/lib/libtcl8.6.so /usr/local/lib/libtcl8.5.so

zebraConfigPath="/usr/local/etc/quagga/zebra.conf"

clear 

pswLineNumber=$(cat $zebraConfigPath | grep -n "password quagga" | cut -d : -f 1)


if [ $pswLineNumber -eq 1 ]
then
startQuagga 20
else
eth0ConfigLineNumber=$(cat $zebraConfigPath | grep -n "interface eth0")	
eth0ConfigLineNumber=$(echo "$eth0ConfigLineNumber" | cut -d : -f 1)
eth0ConfigLineNumber=$((eth0ConfigLineNumber+2))
eth0IpAddress=$(cat $zebraConfigPath | head -$eth0ConfigLineNumber | tail -1 | cut -d " " -f 4 | cut -d "/" -f 1)
eth0IpAddressOctet=( $(echo $eth0IpAddress | awk '{split($0,octet,"."); print octet[1],octet[2],octet[3],octet[4]}') )
eth0Netmask=$(cat $zebraConfigPath | head -$eth0ConfigLineNumber | tail -1 | cut -d " " -f 4 | cut -d "/" -f 2)

vtysh -c 'configure terminal' -c 'interface eth0' -c 'no ip address '$eth0IpAddress'/'$eth0Netmask'' -c 'exit' -c 'router ospf' -c 'no network '${eth0IpAddressOctet[0]}'.'${eth0IpAddressOctet[1]}'.'${eth0IpAddressOctet[2]}'.'0'/'$eth0Netmask' area 1'
echo "Removing old configuration information..."
vtysh -c 'write'

startQuagga 20

fi
