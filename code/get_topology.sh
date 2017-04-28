#!/bin/bash

ip_address=$(cat /usr/local/etc/quagga/zebra.conf | grep "ip address" | cut -d " " -f4 | cut -d / -f1)

echo -e "\nBuilding Up Network Topology..."

first_line_network_state_section=$(vtysh -c "show ip ospf database" | grep -nr "Net Link States" | cut -d : -f 1)

if [ "$first_line_network_state_section" == "" ]; then 
echo -e "\n========== ROUTER R8 HAS TO BE SWITCHED ON ==========!!\n"
clear
exit 1 
fi

last_line_router_state_section=$((first_line_network_state_section-2))

first_line_router_state_section=$((last_line_router_state_section-6))

router_state_section=$(vtysh -c "show ip ospf database" | head -$last_line_router_state_section | tail -$first_line_router_state_section)

routerId=( $(echo "$router_state_section" | awk '{print $1;}') )

community="public"

#array which contains for each router (i) the ip addresses which connect it to other routers via fastEthernet links.
declare -A ipAddrIf
#array which contains for each router the ip address of each interface.
declare -A ipAddress 
#array which contains for each router (i) having at least 1 serial link connection, the ip address(es) of the interface(s) which connects to that(those) neighborn(s) router(s).
declare -A localIpAddrIf
#array which specifies for each designated router (i) the ip address(es) of its interfaces that connect it to other routers via fast-ethernet links.
declare -A attachedRouterIpAddrIf 
#array which contains for each neighborn router (i) the ip address(es) of the interface(s) which connects it to its neighborns.
declare -A p2pRouterIpAddrIf
#array which contains for each router (i) the routerIDs of the other routers connected to itvia fastEthernet links.
declare -A attachedRouterId
#array which contains for each router (i) having at least 1 serial link connection, the routerID of the neighborn router.
declare -A p2pRouterId
#array which contains for a given router the # of routers connected to it via fastEthernet links.
declare -A counter
#array which specifies for each router (i) if it at least 1 serial link connection is available.
declare -A p2p 
#array which counts the # of p2p connections of each router.
declare -A counterNeighborn
#array which contains for a given router the ip address(es) of the interface(s) which connects it to neighborns routers.
declare -A targetIpAddr
#associative array:
# 					index: ip address of an MPLS-TE enabled interface 
#					value: the reserved capacity for establishing TE-tunnels on that interface
declare -A rsvIfB
#associative array:
# 					index: ip address of an MPLS-TE enabled interface 
#					value: the max reserved capacity that each TE-tunnel can request on that interface 
declare -A rsvIfBPerFlow
#associative array:
# 					index: ip address of an MPLS-TE enabled interface 
#					value: the total in use (reserved) capacity on that interface
declare -A totalUsedRsvB
#array which contains the routerID of each active router. Active means it is actually working. 
declare -A activeRouter


for (( i=0; i<${#routerId[@]}; i++ )) 
do
if [ "${routerId[$i]}" == "$ip_address" ]; then 
continue
else
counter[$i]=0
counterNeighborn[$i]=0
p2p[$i]=0
fi
done

acIndex=0;

for (( i=0; i<${#routerId[@]}; i++ )) 
do
if [ "${routerId[$i]}" == "$ip_address" ]; then 
continue
else
#array which contains the ip address and index value which uniquely identify each interface of a router
routerIfConfig=( $(snmpbulkwalk -v2c -c $community ${routerId[$i]} 1.3.6.1.2.1.4.20.1.2) )
numberIf=$((${#routerIfConfig[@]}/4))
for (( j=0; j<$numberIf; j++ )) 
do
#array which contains for each router (i) the ip address of each interface (j) 
ipAddressIf[$i,$j]=$(echo "${routerIfConfig[$((4*j))]}" | awk -F "." '{print $(NF-3)"."$(NF-2)"."$(NF-1)"."$(NF);}')
#array which contains for each router (i) the index value of each interface (j) 
indexIf[$i,$j]=$(echo "${routerIfConfig[$((3+4*j))]}") 
done
if [ "${#indexIf[@]}" -eq 0 ]
then
echo -en "Router ID: ${routerId[$i]} "
continue
fi
#array which for a given router the index values of the MPLS-TE enabled interfaces together with the correspondent reserved capacity
rsvpBConfig=( $(snmpbulkwalk -v2c -c $community ${routerId[$i]} 1.3.6.1.2.1.52.1.1.1.2) )
#array which contains for each router (i) the number of MPLS-TE enabled interfaces
numberTargetIf[$i]=$((${#rsvpBConfig[@]}/4))
for (( j=0; j<${numberTargetIf[$i]}; j++ )) 
do
#array which contains for each router (i) the index values of the MPLS-TE enabled interfaces
targetIfIndex[$i,$j]=$(echo "${rsvpBConfig[$((4*j))]}" | awk -F "." '{print $(NF);}')
#array which contains for each router (i) the reserved capacity of each MPLS-TE enabled interfaces
rsvpIfB[$i,$j]=$(echo "${rsvpBConfig[$((3+4*j))]}")
done
if [ "${#targetIfIndex[@]}" -eq 0 ]
then
echo -en "Router ID: ${routerId[$i]} "
continue
fi
#array which contains for a given router the max reserved capacity that each TE tunnel can request on each MPLS-TE enabled interfaces
rsvpIfBPerFlow=( $(snmpbulkwalk -v2c -c $community ${routerId[$i]} 1.3.6.1.2.1.52.1.1.1.3 | awk -F ": " '{print $2;}') )
if [ "${#rsvpIfBPerFlow[@]}" -eq 0 ]
then
echo -en "Router ID: ${routerId[$i]} "
continue
fi
#array which contains for a given router the max in use reserved capacity on each MPLS-TE enabled interfaces
totalUsedRsvpB=( $(snmpbulkwalk -v2c -c $community ${routerId[$i]} 1.3.6.1.2.1.52.1.1.1.1 | awk -F ": " '{print $2;}') )
if [ "${#totalUsedRsvpB[@]}" -eq 0 ]
then
echo -en "Router ID: ${routerId[$i]} "
continue
fi

activeRouter[$acIndex]=${routerId[$i]}
acIndex=$((acIndex+1))


for (( j=0; j<$numberIf; j++ )) 
do
ipAddress[$i,${indexIf[$i,$j]}]=${ipAddressIf[$i,$j]}
done
for (( j=0; j<${numberTargetIf[$i]}; j++ )) 
do
rsvIfB[${ipAddress[$i,${targetIfIndex[$i,$j]}]}]=${rsvpIfB[$i,$j]}
rsvIfBPerFlow[${ipAddress[$i,${targetIfIndex[$i,$j]}]}]=${rsvpIfBPerFlow[$j]}
totalUsedRsvB[${ipAddress[$i,${targetIfIndex[$i,$j]}]}]=${totalUsedRsvpB[$j]}
done
fi
done

for (( i=0; i<${#activeRouter[@]}; i++ )) 
do
#array which contains for a given router the routerIDs of all the routers connected to it via serial links.
neighborRouterId=( $(vtysh -c 'show ip ospf database router '${activeRouter[$i]}'' | grep "Neighboring Router ID:" | cut -d : -f 2) )
k=0
if [ ${#neighborRouterId[@]} -gt 0 ]; then
#array which contains for each router (i) the # of routers connected via serial links.
counterNeighborn[$i]=${#neighborRouterId[@]}
p2p[$i]=1
for (( j=0; j<${#neighborRouterId[@]}; j++ )) 
do
p2pRouterId[$i,$j]=${neighborRouterId[$j]}
neighbornRouterIdLineNumber=$(vtysh -c 'show ip ospf database router '${activeRouter[$i]}'' | grep -n "Neighboring Router ID: ${neighborRouterId[$i]}" | cut -d : -f 1)
neighbornRouterIdLineNumber=$((neighbornRouterIdLineNumber+1))
localIpAddrIf[$i,$j]=$(vtysh -c 'show ip ospf database router '${activeRouter[$i]}'' | head -n $neighbornRouterIdLineNumber | tail -1 | cut -d : -f 2 | cut -d " " -f 2)
targetIpAddr[$k]=${localIpAddrIf[$i,$j]}
k=$((k+1))
neighbornRouterIdLineNumber=$(vtysh -c 'show ip ospf database router '${neighborRouterId[$j]}'' | grep -n "Neighboring Router ID: ${activeRouter[$i]}" | cut -d : -f 1)
neighbornRouterIdLineNumber=$((neighbornRouterIdLineNumber+1))
p2pRouterIpAddrIf[$i,$j]=$(vtysh -c 'show ip ospf database router '${neighborRouterId[$j]}'' | head -n $neighbornRouterIdLineNumber | tail -1 | cut -d : -f 2 | cut -d " " -f 2)
done
fi
#array which specifies for each router (i) the ip address(es) of interfaces of other router(s) connected to it via fast-ethernet links.
designatedRouterIpAddress=( $(vtysh -c 'show ip ospf database router '${activeRouter[$i]}'' | grep "Designated Router address:" | cut -d : -f 2) )
#array which contains for each router (i) having at least 1 fast-ethernet connection, the ip address(es) of the interface(s) which connects to that(those) neighborn(s) router(s).
routerIfAddress=( $(vtysh -c 'show ip ospf database router '${activeRouter[$i]}'' | grep "Router Interface address:" | cut -d : -f 2) ) 
#routerIfAddress contains also the ip addresses which connects the activeRouter[i] to its neighborn routers, but we just need the ip addresses that connects it to other routers via fastethernet links.
for (( j=0; j<${#routerIfAddress[@]}; j++ )); do
for (( w=0; w<${#targetIpAddr[@]}; w++ )); do
if [ "${routerIfAddress[$j]}" == "${targetIpAddr[$w]}" ]; then
routerIfAddress=( "${routerIfAddress[@]:0:$j}" "${routerIfAddress[@]:$((j + 1))}" )
break
fi
done
done
for (( j=0; j<${#designatedRouterIpAddress[@]}; j++ )) 
do
if [ "${designatedRouterIpAddress[$j]}" != "${routerIfAddress[$j]}" ]; then 
ipAddrIf[$i,${counter[$i]}]=${routerIfAddress[$j]} 
attachedRouterId[$i,${counter[$i]}]=$(vtysh -c 'show ip ospf database network '${designatedRouterIpAddress[$j]}'' | grep "Advertising Router:" | cut -d : -f 2 | cut -d " " -f 2) 
attachedRouterIpAddrIf[$i,${counter[$i]}]=${designatedRouterIpAddress[$i,$j]}
for (( w=0; w<${#activeRouter[@]}; w++ )) 
do
if [ "${activeRouter[$w]}" == "${attachedRouterId[$i,${counter[$i]}]}" ]; then 
attachedRouterId[$w,${counter[$w]}]=${activeRouter[$i]}
attachedRouterIpAddrIf[$w,${counter[$w]}]=${routerIfAddress[$j]}
ipAddrIf[$w,${counter[$w]}]=${attachedRouterIpAddrIf[$i,${counter[$i]}]}
counter[$i]=$((counter[$i]+1))
counter[$w]=$((counter[$w]+1))
break
fi
done
fi
done
done

#We create the file on which write the network topology
file="./topology.txt"
if [ -f "$file" ]
then
rm -rf $file 
fi

#We write the # of available routers in the networks.
echo "$acIndex" >> topology.txt
for (( i=0; i<${#activeRouter[@]}; i++ )) 
do
for (( j=0; j<${counter[$i]}; j++ )) 
do
echo "${activeRouter[$i]} ${ipAddrIf[$i,$j]} ${attachedRouterId[$i,$j]} ${attachedRouterIpAddrIf[$i,$j]} ${rsvIfB[${ipAddrIf[$i,$j]}]} ${rsvIfBPerFlow[${ipAddrIf[$i,$j]}]} $((rsvIfB[${ipAddrIf[$i,$j]}]-totalUsedRsvB[${ipAddrIf[$i,$j]}]))" >> topology.txt
done
if [ "${p2p[$i]}" -ne 0 ]; then 
for (( w=0; w<${#counterNeighborn[$i]}; w++ )); do
echo "${activeRouter[$i]} ${localIpAddrIf[$i,$w]} ${p2pRouterId[$i,$w]} ${p2pRouterIpAddrIf[$i,$w]} ${rsvIfB[${localIpAddrIf[$i,$w]}]} ${rsvIfBPerFlow[${localIpAddrIf[$i,$w]}]} $((rsvIfB[${localIpAddrIf[$i,$w]}]-totalUsedRsvB[${localIpAddrIf[$i,$w]}]))" >> topology.txt
done
fi
done

#clear
