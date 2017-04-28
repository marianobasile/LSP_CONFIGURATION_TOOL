#!/bin/bash
acquireUsername() {

while true
do
echo ""
echo -en "Enter username for telnet authentication on $1: "
read username
if [[ $username =~ ^[A-Za-z]+$ ]]
then
echo -en "\nConfirm username:"
read usrname
if [ "$usrname" == "$username" ]
then
break
else
acquireUsername $1
break
fi
else
echo -e "\nInvalid username!"
fi
done
}

acquirePswd() {

while true
do
echo ""
echo -en "Enter password for telnet authentication on $1: "
read password
if [[ $password =~ ^[a-zA-Z0-9]+$ ]]
then
echo -en "\nConfirm password:"
read psw
if [ "$password" == "$psw" ]
then
break
else
acquirePswd $1
break
fi
else
echo -e "\nInvalid password!"
fi
done
}

acquireUsername $1

acquirePswd $1

while true
do
echo ""
echo -en "Enter a name for the tunnel: "
read tunnelName
if [[ $tunnelName =~ ^[a-zA-Z0-9]+$ ]]
then
break
else
echo -e "\nInvalid tunnel name!"
fi
done

while true
do
echo ""
echo -en "Enter a name for the LSP: "
read LSP
if [[ $LSP =~ ^[a-zA-Z]+$ ]]
then
break
else
echo -e "\nInvalid path name!"
fi
done

clear

./configure_tunnel.exp $username $password $1 $2 $3 $4 $tunnelName $LSP

exit $? 
