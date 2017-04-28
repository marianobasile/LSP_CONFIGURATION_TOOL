#define NULL (void*)0
#define PERC_AVAIL_GAP 0.01  /*1%*/
/*
*******************************************LINK****************************************************
topologic link for each router interface!
	srcID is the source router ID (Loopback Address)
	srcIntIPAddr is one of the source router network interfaces

	destID is the destination router ID (Loopback Address)
	destIntIPAddr is one of the destination router network interfaces

	band is the residual bandwith on the link srcID-destID
	maxBand is the max bandwidth reserved on the link srcID-destID
	maxBandPerFlow is the max bandwidth reservable per flow on the link srcID-destID
*/
	struct link{
		char srcID[16];			/*Ipv4 format:  xxx.xxx.xxx.xxx = 15 + '\0' = 16 char */
		char srcIntIPAddr[16];

		char destID[16];
		char destIntIPAddr[16];

		int band;
		int maxBand;
		int maxBandPerFlow;

		struct link* next;
	};

/*____________________________________________GLOBAL VARIABLES________________________________________*/
/*Array of links per each router in the network*/
	struct link** network;
	int nrouters;
/*____________________________________________________________________________________________________*/

/*Initialize the array of link pointers*/
	void initNetwork();

/*Insert at head a new link in network[networkIndex] list!*/
	void addLink(int networkIndex ,char* src, char* srcInt, char* dest, char* destInt, int band, int maxB, int MaxBPF);

/*Load topology from the script "get_topology.sh"*/
	void loadTopology();

/*Free the dynamic memory allocated for the network array*/
	void freeTopology();

/*Refresh the Topology by running the "get_topology.sh" script and retrieving again the network topology*/
	void refreshTopology();

/*Utility function to search the Index in the network array for a given link source*/
	int searchLinkArrayIndexBySrcID(char* id);

/*
**********************************************NODE**************************************************
node of the tree of possible paths from a given root (the Ingress router)!
	child-slibling tree.
	root is the sourceNode (Ingress): in its siblings are all its interfaces.
*/
	struct node
	{
		struct link* linkPointer;	/*pointer to a "link": where are stored all the info*/

		struct node* father;
		struct node* child;
		struct node* sibling;
	};
/*
Example:
	fake ->NULL
	|
	\/
	[source-link1] -> [source-link2] -> [source-link3]
	|
	\/
	[nextHop-linkX] -> [nextHop-linkY] -> [nextHop-linkZ]
			     |			  |
			     \/			  \/
			     [dest-linkW]	  [nextHop-linkY]
						  |
						  \/
						  [dest-linkX]


	this represent:		[fake]
				|
				[routerA]
				|
				[routerB]
				|	 \
				[routerC] [routerD]
						   \
						   [routerC]
*/

/*Initialize the tree of node pointers (is created the "root fake node")*/
	void initPathTree(struct node** tree);

/*Add a child in the Tree*/
	struct node* addChild(struct node** f, struct link* l);

/*Add a sibling in the Tree*/
	struct node* addSibling(struct node** lastSibling, struct link* l);

/*Recursively check if in the father is present the currentLink (that means i'm going on a visited Router)*/
	int checkForLoop(struct node* currentNode, struct link* currentLink);

/*Build the Tree from topological informations  (if firstChild == 1 --> addChild() )*/
	void buildPathTree(struct node** root, char* srcID, char* destID,  int firtsChild);

/*Debuggin function to show the tree built!*/
	void printPathTree(struct node* root);

/*Freeing the memory allocated for the tree (after computed the best path!)*/
	void freePathTree(struct node** root);
/* 
********************************************PATH****************************************************
*/

/*Recursively compute a node* (bestLeaf) that is the bestLeaf (from which get the path!)
best w.r.t. maximum percentual of availabile band along the path and in case of equality (with a tollerance of 0.1%) w.r.t. min number of hops!
*/
	void computeBestLeaf(struct node* root, char* destID, int bandw, float availUtilization, int pathLen, struct node** bestLeaf,float *bestAvgAvailUtilization, int *bestLen);


/*Locally update the link's band refered to the path allocated for the tunnel*/
	void updateLinkBand(struct node* leaf, int band);


/*Struct to crete a list with the interface addresses --> to configure the lsp tunnel on the router!*/
	struct hop{
		char* interfaceIPAddr;

		struct hop* next;
	};

/*Initialize the list of hop (the chosen path!) by adding a fake hop*/
	void initPath(struct hop** path);

/*Create the list of hop, from the besstLeaf (the lsp tunnel!)*/
	void createLSP(struct node* bestLeaf, struct hop** path);

/*Show the list of hop (interface addresses)*/
	void printLSP(struct hop* path, char* ingress, char* egress, int reqBand);

/*Freeing the memory allocated for the list*/
	void freePath(struct hop** path);

/* 
******************************************SCRIPT**************************************************
*/
/*Call the script to configure Quagga on a Core 4.7 machine and the script to retrieve the topology*/
	void configureQuagga();

/*Call the script to configure the found Tunnel on the Ingress Router*/
	int configureTunnelScript(char* ingress, char* egress, int reqBand, struct hop* path);

/*Call the script to get the available LSPs on the Ingress Router*/
	void getLSP(char* ingress);

/* 
******************************************SERVICES**************************************************
*/

/*Show to the user the network topology*/
	void showNetworkTopology();

/*Show to the user the connections of a give routerID*/
	void showRouterConnections();

/*Read user-lsp inputs*/
	void readRequiredLSP(char** ingress, char** egress, int* reqBand);

/*Create the LSP paths*/
	void buildLSP();

/*Show the LSPs*/
	void showLSP();

/* 
******************************************UTILITY**************************************************
*/

/*Clean the stdin after user input mistake*/
	int cleanStdin();

/*Show the tool help menu*/
	void toolHelp();

/*Parse user-menu inputs*/
	void parseInput(int input);

/*User GUI*/
	void showGUI();