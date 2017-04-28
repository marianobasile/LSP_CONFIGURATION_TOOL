Accertarsi di aver scaricato i file in un ambiente provvisto del simulatore GSN3.

1)Importare la rete simulata
-->Eseguire GSN3.
-->Caricare la rete "project_network.gsn3" presente nella cartella "rete_GSN3"
-->Disconnettere il link che collega R8 con la VM con Quagga

2)Predisporre la nuove VM da importare
--> andare in /home/user/gns3/images/virtualbox/ ed elimare il file .vdi linux-core-4.7.7-openswitch...
--> copiare il nuovo file .vdi
--> aprire virtualbox e creare una nuova macchina virtuale, selezionando come immagine del so il file .vdi di cui sopra
--> creata la vm cliccare su edit --> network --> adapter1 --> selezionare not attached



3)Importare la VM con il tool installato
--> Edit  -->  Preferences --> VirtualBox VMs  --> New --> selezionare la VM appena predisposta --> Finsh
--> importare la VM, ora presente tra i devices
--> attaccare la VM ad un router (i.e R8) mediante un link ethernet


4)Eseguire il tool
-->avviare l'host (la VM), l'operazione può richiedere diversi minuti a seconda delle risorse del calcolatore
-->quando l'host è operativo, cioè quando è possibile eseguire l'autenticazione, avviare i router ed eventualmente gli host della rete
-->autenticarsi sulla VM: tc
-->spostarsi nella directory /mnt/sda1/tce/project
-->eventualmente compilare il file lsp.c: gcc -o lsp lsp.calcolatore
-->eseguire il tool in modalità root: sudo ./lsp


5)Configurazione iniziale
-->all'avvio del tool è richiesta la configurazione dell'interfaccia eth0: inserire un indirizzo ip (i.e. 192.168.8.2 se connesso ad R8)
-->successivamente inserire una maschera di rete valida (i.e. 24 se connesso ad R8)
-->attendere il caricamento delle informazioni topologiche (operazione che richiede qualche minuto, a seconda delle risorse disponibili del calcolatore)
