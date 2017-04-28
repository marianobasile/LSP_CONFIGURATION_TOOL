#include "lsp.h" 
#include <stdlib.h> 
#include <errno.h> 
#include <stdio.h> 
#include <unistd.h> 
#include <string.h> 
#include <string.h> 
#include <math.h>

/*
*******************************************LINK****************************************************
*/
void initNetwork(){

	int i;

	network = (struct link**)malloc(nrouters*sizeof(struct link*));
	if(network == NULL){
		printf("Error allocating a network: %s\n", strerror(errno));
		exit(-1);
	}

	for(i=0; i<nrouters; i++)
		network[i] = NULL;
}

void addLink(int networkIndex ,char* src, char* srcInt, char* dest, char* destInt, int band, int maxB, int maxBPF){

	struct link* app;

	app = (struct link*)malloc(sizeof(struct link));
	if(app == NULL){
		printf("Error allocating a link: %s\n", strerror(errno));
		exit(-1);
	}
	strcpy( app->srcID, src);
	strcpy(app->srcIntIPAddr, srcInt);
	strcpy(app->destID, dest);
	strcpy(app->destIntIPAddr, destInt);
	app->band = band;
	app->maxBand = maxB;
	app->maxBandPerFlow = maxBPF;
	app->next = network[networkIndex];
	network[networkIndex] = app;
}


void loadTopology(){

	char srcID[16], srcInterface[16], destID[16], destInterface[16], currentID[16] = "\n";
	int band, maxB, maxBPF, index = 0;

	char* topology = "topology.txt";
	FILE* f = fopen( topology , "r");
	if( f == NULL){
		printf("\nError opening %s: %s\n", topology, strerror(errno));
		exit(-1);
	}

	/*read nrouters*/
	fscanf(f, "%d", &nrouters);

	initNetwork();

	while(fscanf(f, "%s %s %s %s %d %d %d\n", srcID, srcInterface, destID, destInterface, &maxB, &maxBPF, &band) == 7){
		if( strcmp(currentID, "\n") == 0 )
			strcpy( currentID, srcID);
		if( strcmp(currentID, srcID) != 0 ){
			index++;
			strcpy(currentID, srcID);
		}
		addLink( index, srcID, srcInterface, destID, destInterface, band, maxB, maxBPF);
	}

	fclose(f);	
}

void freeTopology(){
	
	int i;
	struct link* p;

	for(i=0; i<nrouters; i++){
		while(network[i] != NULL){
			p = network[i];
			network[i] = network[i]->next;
			free(p);
		}
	}

	free(network);
}

void refreshTopology(){
	system("./get_topology.sh");
	freeTopology();
	loadTopology();
}

int searchLinkArrayIndexBySrcID(char* id){

	int i;

	for(i=0; i<nrouters; i++){
		if( strcmp(network[i]->srcID, id) == 0 )
			return i;
	}

	return -1;
}

/*
**********************************************NODE**************************************************
*/
void initPathTree( struct node** tree){

	int i;

	*tree =  (struct node*)malloc(sizeof(struct node));
	if(*tree == NULL){
		printf("Error allocating tree: %s\n", strerror(errno));
		exit(-1);
	}
	(*tree)->linkPointer = NULL;
	(*tree)->father = NULL;
	(*tree)->child = NULL;
	(*tree)->sibling = NULL;
}

struct node* addChild( struct node** f, struct link* l ){

	struct node* app;
	app = (struct node*)malloc(sizeof(struct node));
	if(app == NULL){
		printf("Error allocating a node child: %s\n", strerror(errno));
		exit(-1);
	}
	app->linkPointer = l;
	app->father = *f;
	app->child = NULL;
	app->sibling = NULL;

	(*f)->child = app;

	return app;
}

struct node* addSibling( struct node** lastSibling, struct link* l ){
	
	struct node* app = NULL;
	app = (struct node*)malloc(sizeof(struct node));
	if(app == NULL){
		printf("Error allocating a node sibling: %s\n", strerror(errno));
		exit(-1);
	}
	app->linkPointer = l;
	app->father = (*lastSibling)->father;
	app->child = NULL;
	app->sibling = NULL;

	(*lastSibling)->sibling = app;

	return app;
}

/*Recursively check if in the father is present the currentLink (that means i'm going on a visited link)*/
int checkForLoop( struct node* currentNode, struct link* currentLink ){

	/*Check if it is the root (the root is a fake node: root->child point to "ingress router")*/
	if( currentNode->father == NULL ) return 0;

	/*Check if i'm going (destID) to a router yet visited (srcID)*/
	if( strcmp( currentNode->linkPointer->srcID, currentLink->destID ) == 0 ) return -1;

	checkForLoop( currentNode->father, currentLink );
}

/*Build the Tree from topological informations*/
void buildPathTree( struct node** root, char* srcID, char* destID, int fistChild){
	
	int index, fChild = fistChild;
	struct node* currentNode = *root;
	struct link* currentLink;

	index = searchLinkArrayIndexBySrcID( srcID );
	/*Router not found: maybe it has been detected down in the network in the topolofy refresh*/
	if(index == -1){
		//printf("\nsearchLinkArrayBySrcID: element (Router) not found!\n");
		return;
	}
	currentLink = network[index];

	/*termination condition: i've reached my destination (destID)*/
	if(strcmp( srcID, destID ) == 0)	return;


	/*Building firt child  -> while on currentLink because we must avoid loops!*/
	while( currentLink != NULL ){
		if( checkForLoop( currentNode, currentLink ) == 0 ){

			if(fChild == 1){
				/*Add the first child*/
				currentNode = addChild( &currentNode, currentLink );
			
				/*Unset fChild --> for currentLink->next need to add sibling!*/
				fChild = 0;

				/*recursively build on the child created*/
				buildPathTree( &currentNode, currentLink->destID, destID, 1);
			}
			else{
				/*Add a sibling*/
				currentNode = addSibling( &currentNode, currentLink );

				/*recursively build on the child created*/
				buildPathTree( &currentNode, currentLink->destID, destID, 1);
			}
		}
		currentLink = currentLink->next;
	}
	/*i've explored all the interfaces of the current router*/
	return;
}


void printPathTree( struct node* root ){

	printf("[%s - %s] ->", root->linkPointer->srcID, root->linkPointer->destID);

	if( root->child != NULL )
		printPathTree( root->child );
	if( root->sibling != NULL ){
		printf("\n|\n\t--> sibling of [%s]", root->linkPointer->srcID);
		printPathTree( root->sibling );
	}

}

void freePathTree( struct node** root ){

	if( (*root)->sibling != NULL )
		freePathTree( &((*root)->sibling) );

	if( (*root)->child != NULL )
		freePathTree( &((*root)->child) );

	free( *root );
}


/* 
********************************************PATH****************************************************
*/
void computeBestLeaf(struct node* root, char* destID, int bandw, float availUtilization, int pathLen, struct node** bestLeaf,float *bestAvgAvailUtilization, int *bestLen){

	float actualAvailUtilization,temp,actualAvgAvailUtilization;
	
	temp = ((float)(root->linkPointer->band - bandw)) / ((float)root->linkPointer->maxBand);	
	actualAvailUtilization = temp + availUtilization;	
	
	if(strcmp(root->linkPointer->destID, destID) == 0 && root->linkPointer->band >= bandw && root->linkPointer->maxBandPerFlow >= bandw){

		actualAvgAvailUtilization = actualAvailUtilization/(pathLen+1);

		/*Found a Better Path (Leaf reached is better than the actual best Leaf)*/
		if( (*bestAvgAvailUtilization == -1 && *bestLen == -1) || 
			(actualAvgAvailUtilization - *bestAvgAvailUtilization > PERC_AVAIL_GAP) || 
			((fabs(actualAvgAvailUtilization - *bestAvgAvailUtilization) <= PERC_AVAIL_GAP) && (pathLen + 1 < *bestLen)) ){

			*bestLeaf = root;
			*bestAvgAvailUtilization = actualAvgAvailUtilization;	

			*bestLen = pathLen + 1;
		}
	}
	/*Descend the Tree on the child*/
	else if(root->linkPointer->band >= bandw && root->linkPointer->maxBandPerFlow >= bandw && root->child != NULL)
		computeBestLeaf(root->child, destID, bandw, actualAvailUtilization, pathLen + 1, bestLeaf,bestAvgAvailUtilization, bestLen);

	/*Explore if siblings (another link on the current router)*/
	if(root->sibling != NULL)
		computeBestLeaf(root->sibling,destID,bandw,availUtilization,pathLen, bestLeaf,bestAvgAvailUtilization, bestLen );
}

void updateLinkBand(struct node* leaf, int band){
	
	int index;
	struct link* p;
	
	if(leaf == NULL)	return;
	
	/*updating the sender interface*/
	leaf->linkPointer->band -= band;
	
/*
//updating also the receiver interface
index = searchLinkArrayIndexBySrcID( leaf->linkPointer->destID );
p = network[index];
while( strcmp(p->destID,leaf->linkPointer->srcID) != 0)
	p = p->next;
p->band -= band;
*/

	/*recursively call until my father is not the fake node!*/
	if(leaf->father->linkPointer != NULL)
		updateLinkBand( leaf->father, band);
}

void initPath(struct hop** path){

	*path = (struct hop*)malloc(sizeof(struct hop));
	(*path)->interfaceIPAddr = NULL;
	(*path)->next = NULL;
}

void createLSP(struct node* bestLeaf, struct hop** path){

	struct hop* app;

	if(bestLeaf->father == NULL)	return;

	app = (struct hop*)malloc(sizeof(struct hop));
	app->interfaceIPAddr = bestLeaf->linkPointer->destIntIPAddr;
	app->next = *path;
	*path = app;

	createLSP(bestLeaf->father, path);
}


void printLSP(struct hop* path, char* ingress, char* egress, int reqBand){

	struct hop* h;

	printf("\n\n[Path from %s to %s (%d bps) allocated]:", ingress, egress, reqBand);
	h = path;
	while(h->next != NULL){
		printf("\n\t%s", h->interfaceIPAddr);
		h = h->next;
	}
	printf("\n\n");
}
	
void freePath(struct hop** path){

	if((*path)->next != NULL)
		freePath(&((*path)->next));

	free(*path);	
}
/* 
******************************************SCRIPT**************************************************
*/

void configureQuagga(){

	char* quaggaScript = "./quagga_config.sh";
	system(quaggaScript);
}

int configureTunnelScript(char* ingress, char* egress, int reqBand, struct hop* path){
	
	struct hop* p = path;
	int ret, len = 0;
	char* tunnelScript = "./configure_tunnel.sh";
	char band[18];

	snprintf(band, 18, "%d", reqBand);		/*reqBand is Kbps*/

	/*creating the call for the script with the argument to configure the tunnel*/
	len += strlen(tunnelScript);
	len += strlen(ingress) + 1;
	len += strlen(egress) + 1;
	len += strlen(band) + 1;

	while( p->next != NULL){
		len += strlen(p->interfaceIPAddr) + 1;
		p = p->next;
	}
	char script[len];

	p = path;

	strcpy(script, tunnelScript);
	strcat(script, " ");
	strcat(script, ingress);
	strcat(script, " ");
	strcat(script, egress);
	strcat(script, " ");
	strcat(script, band);
	strcat(script, " ");

	strcat(script, p->interfaceIPAddr);
	
	p = p->next;
	while( p->next != NULL){
		strcat(script, "/");
		strcat(script, p->interfaceIPAddr);
		p = p->next;
	}
	ret = system(script);

	return ret;
}

void getLSP(char* ingress){
	
	int len = 0;

	char* getLspScript = "./show_tunnel.sh";

	len += strlen(getLspScript)+1;
	len += strlen(ingress);

	char script[len];

	strcpy(script, getLspScript);
	strcat(script, " ");
	strcat(script, ingress);

	system(script);

}

/* 
******************************************SERVICES**************************************************
*/
void showNetworkTopology(){

	int i;
	struct link* p;

	printf("\n\n\t\t%15s\t%18s\t%12s", "Next-Hop", "Avail B.(Kbps)", 
	"Reser B.(Kbps)" );

	for(i=0; i<nrouters; i++){
		p = network[i];
		printf("\n%15s:", network[i]->srcID);

		while(p != NULL){
			printf("\n\t\t%16s\t%6d\t%17d",p->destID,(p->band/1000), (p->maxBand/1000) );
			p = p->next;
		}
		printf("\n");
	}
	printf("\n");
	showGUI();
}

void showRouterConnections(){

	int i;
	struct link* p;
	char router[16];

	printf("\nInsert RouterID:\t");
	scanf("%s", router);

	while( (i = searchLinkArrayIndexBySrcID( router )) == -1 ){

		if( strlen(router) == 1 && strcmp(router, "q") == 0){
			showGUI();
			return;
		}
		printf("\n\nPlease insert a correct Router Address! [q + ENTER to back the menu]\n\n");
		printf("\nInsert RouterID:\t");
		scanf("%s", router);
	}

	p = network[i];

	printf("\n\n%15s\t%15s\t%18s\t%18s", "Next-Hop", "via inter.","Avail B.(Kbps)" , "Reser B.(Kbps)" );

	//printf("\n%15s linked to:", p->srcID);
	while(p != NULL){
		printf("\n%15s\t%15s\t%18d\t%18d", p->destID, p->srcIntIPAddr, (p->band/1000) , (p->maxBand/1000) );
		p = p->next;
	}
	printf("\n\n");

	showGUI();
}


/*Read user inputs: requested lsp*/
void readRequiredLSP(char** ingress, char** egress, int* reqBand){
	
	int i, len, ret;
	char c;
	
	/*Reading ingress router*/
	*ingress = (char*)malloc(16*sizeof(char));
	if( *ingress == NULL){
		printf("Error allocating ingress: %s\n", strerror(errno));
		exit(-1);
	}
	printf("\nInsert Ingress RouterID:\t");
	scanf("%s", *ingress);
	while( searchLinkArrayIndexBySrcID( *ingress ) == -1 ){
		if( strlen(*ingress) == 1 && strcmp(*ingress, "q") == 0){
			free(*ingress);
			showGUI();
			return;
		}
		printf("\n\nPlease insert a correct Ingress Router Address! [q + ENTER to back the menu]\n\n");
		printf("\nInsert Ingress RouterID:\t");
		scanf("%s", *ingress);
	}
	
	/*Reading egress router*/
	*egress = (char*)malloc(16*sizeof(char));
	if( *egress == NULL){
		printf("Error allocating egress: %s\n" , strerror(errno));
		exit(-1);
	}
	printf("\nInsert Egress RouterID: \t");
	scanf("%s", *egress);
	while( searchLinkArrayIndexBySrcID( *egress ) == -1 || strcmp( *ingress, *egress) == 0 ){

		if( strlen(*egress) == 1 && strcmp(*egress, "q") == 0){
			free(*ingress);
			free(*egress);
			showGUI();
			return;
		}
		printf("\n\nPlease insert a correct Ingress Router Address! [q + ENTER to back the menu]\n\n");
		printf("\nInsert Egress RouterID: \t");
		scanf("%s", *egress);
	}

	/*Reading required bandwidth*/
	printf("\nInsert Bandwidth (Kbps):\t");
	ret = scanf("%d%c", reqBand, &c );
	while( (ret != 2 || c != '\n') || *reqBand <= 0 ){

		if(ret != 2 || c != '\n'){
			cleanStdin();
			printf("\n\nPlease insert a number!\n\n");
		}
		else	printf("\n\nPlease insert a positive number!\n\n");
		printf("\nInsert Bandwidth (Kbps):\t");
		ret = scanf("%d%c", reqBand, &c);
	}

	*reqBand = (*reqBand)*1000;		/*converting in bps*/
}

void buildLSP(){

	/*Tree per the required tunnel (tree[i] is a fake node! tree[i]->child is the starting node!)*/
	struct node* tree = NULL;
	/*Best Leaf */
	struct node* bestLeaf = NULL;
	float bestCost;
	int bestLen;
	/*Selected Path: list of hop forming the path*/
	struct hop* path = NULL;

	int i, ret, ntunnels, reqBand;;
	char* ingress, *egress;
	
	/*interacting with the user*/
	readRequiredLSP( &ingress, &egress, &reqBand );

	/*initialize data structures dynamically allocated*/
	initPathTree(&tree);
	initPath(&path);

	/*Updating informations*/
	refreshTopology();

	if( searchLinkArrayIndexBySrcID(ingress) == -1 || searchLinkArrayIndexBySrcID(egress) == -1){
		printf("\nNetwork is changed! Ingress or egress routers are gone down!\n");
	}
	else{
		/*Building a path tree for each ingress (of the tunnel!)*/
		buildPathTree( &tree, ingress, egress, 1 );	

		/*Computing the paths w.r.t required bandwidth*/
		bestCost = -1;
		bestLen = -1;
		computeBestLeaf( tree->child, egress, reqBand, 0, 0, &bestLeaf,&bestCost, &bestLen );


		if( bestLeaf != NULL){
			/*Asembling the list of hops (for the tunnel configuration!)*/
			createLSP(bestLeaf, &path);

			ret = configureTunnelScript(ingress, egress, reqBand/1000, path);	/*converting reqBand in Kbps!*/

			if(ret == 0){
				/*Updating the link band w.r.t. the new tunnel (best path found)*/
				updateLinkBand( bestLeaf, reqBand );				

				/*showing built path (NOTE: path's last element (hop*) is fake!)*/
				printLSP( path, ingress, egress, reqBand );
			}else{
				printf("\nPath cannot be configured!\n");
			}

		}
		else
			printf("\nNo path has been found!\n");
	}
	
	/*Freeing dynamic memory*/
	freePath( &path );
	freePathTree( &tree );
	free(ingress);
	free(egress);

	showGUI();
}

void showLSP(){

	char* ingress;
	
	/*Reading ingress router*/
	ingress = (char*)malloc(16*sizeof(char));
	if( ingress == NULL){
		printf("Error allocating ingress: %s\n", strerror(errno));
		exit(-1);
	}
	printf("\nInsert Ingress RouterID:\t");
	scanf("%s", ingress);
	while( searchLinkArrayIndexBySrcID( ingress ) == -1 ){
		if( strlen(ingress) == 1 && strcmp(ingress, "q") == 0){
			free(ingress);
			showGUI();
			return;
		}
		printf("\n\nPlease insert a correct Ingress Router Address! [q + ENTER to back the menu]\n\n");
		printf("\nInsert Ingress RouterID:\t");
		scanf("%s", ingress);
	}

	/*Updating informations*/
	refreshTopology();

	if( searchLinkArrayIndexBySrcID(ingress) == -1)
		printf("\nNetwork is changed! Ingress router is down!\n");
	else
		getLSP(ingress);

		
	showGUI();
}

/* 
******************************************UTILITY**************************************************
*/

/*Cleaning the stdin when user inputs wrong*/
int cleanStdin(){
	while(getchar() != '\n');
	return 1;
}

/*Show help menu*/
void toolHelp() {
	printf( "\n========== Welcome to Tool Help Tool!===========\n\n\tAvailable commands:");
	printf( "\n1) Show Network Topology\n2) Show Router Connections\n3) Create Tunnel MPLS-TE\n4) Refresh Network Topology\n5) Show LSP MPLS-TE\n6) Exit");
	printf("\nEnter your option and press [ENTER]: ");
}

/*Process the inputs*/
void parseInput(int input){
	if(input == 1) 
		showNetworkTopology();
	if(input == 2)
		showRouterConnections();
	if(input == 3) 
		buildLSP();
	if(input == 4){
		refreshTopology();
		showGUI();
	}
	if(input == 5)
		showLSP();
	if(input == 6) {
		freeTopology();
		exit (-1);
	}	
}

/*Display the GUI and process the inputs*/
void showGUI() {

	char c;
	int input, ret;

	toolHelp();

	ret = scanf("%d%c", &input, &c);
	while( ((ret != 2 || c != '\n') && cleanStdin()) || input < 1 || input > 6){

		printf("\n\nInvalid input! Please insert a number between 1 and 6!\n\n");
		toolHelp();
		ret = scanf("%d%c", &input, &c);
	}

	parseInput(input);
}

/*
**********************************************MAIN**************************************************
*/


int main(int argc, char* argv[]){

	/*Calling the script to configure quagga*/
	configureQuagga();
	
	/*Loading the topology from the file created by the "get_topology.sh script*/
	loadTopology();

	/*Showing the tool-menu to interact with the user*/
	showGUI();
}
