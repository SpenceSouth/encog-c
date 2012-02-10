#include "encog-cmd.h"
#include <string.h>

char parsedOption[MAX_STR];
char parsedArgument[MAX_STR];

void Usage() {
	puts("\nUsage:\n");
	puts("encog xor");
	puts("encog benchmark");
	puts("encog train [eg file] [egb file]");
	puts("encog egb2csv [egb file] [csv file]");
	puts("encog csv2egb [csv file] [egb file]");
	puts("");
	puts("Options:");
	puts("");
	puts("/input:## The number of inputs.");
	puts("/ideal:## The number of ideals.");
	puts("/records:## The number of ideals.");
	puts("/iterations:## The number of ideals.");
	puts("/threads:## The number of threads.");
}

void ParseOption(char *str)
{
	char *ptr;

	if( *str=='/' || *str=='-' ) {
		str++;
	}

	ptr = strchr(str,':');

	if( ptr!=NULL ) {
		strncpy(parsedOption,str,MIN(MAX_STR,ptr-str));
		strncpy(parsedArgument,ptr+1,MAX_STR);
	} else {
		strncpy(parsedOption,str,MAX_STR);
		*parsedArgument = 0;
	}
}

void RunBenchmark(INT inputCount, INT idealCount, INT records, INT iterations ) {
	ENCOG_DATA *data;
	ENCOG_NEURAL_NETWORK *net;
	ENCOG_TRAIN_PSO *pso;
	INT i;
	time_t startTime;
	time_t endTime;
	int elapsed;

	if( inputCount==-1 ) {
		inputCount = 10;
	}

	if( idealCount==-1 ) {
		idealCount = 1;
	}

	if( records==-1 ) {
		records = 10000;
	}

	if( iterations==-1 ) {
		iterations = 100;
	}
	
	printf("\nPerforming benchmark\n");
	printf("Input Count: %i\n",inputCount);
	printf("Ideal Count: %i\n",idealCount);
	printf("Records: %i\n",records);
	printf("Iterations: %i\n",iterations);

	data = EncogDataGenerateRandom(inputCount,idealCount,records,-1,1);

	net = EncogNetworkNew();
	EncogErrorCheck();
    EncogNetworkAddLayer(net,inputCount,&EncogActivationTANH,1);
    EncogNetworkAddLayer(net,50,&EncogActivationTANH,1);
    EncogNetworkAddLayer(net,idealCount,&EncogActivationTANH,1);
    EncogNetworkFinalizeStructure(net);
	EncogErrorCheck();

	EncogNetworkRandomizeRange(net,-1,1);

	pso = EncogTrainPSONew(30, net, data);
	EncogErrorCheck();

	startTime = time(NULL);

	for(i=0;i<iterations;i++) {
		EncogTrainPSOIterate(pso);
		EncogErrorCheck();
	}

	endTime = time(NULL);
	
	elapsed = (int)(endTime - startTime);

	printf("Benchmark time(seconds): %i\n",elapsed);
}

void XORTest() {
	    /* local variables */
    char line[MAX_STR];
    int i;
    REAL *input,*ideal;
    REAL output[1];
    float error;
    ENCOG_DATA *data;
    ENCOG_NEURAL_NETWORK *net;
    ENCOG_TRAIN_PSO *pso;

/* Load the data for XOR */
    data = EncogDataCreate(2, 1, 4);
	EncogErrorCheck();
    EncogDataAdd(data,"0,0,  0");
    EncogDataAdd(data,"1,0,  1");
    EncogDataAdd(data,"0,1,  1");
    EncogDataAdd(data,"1,1,  0");

/* Create a 3 layer neural network, with sigmoid transfer functions and bias */

    net = EncogNetworkFactory("basic", "2:B->SIGMOID->2:B->SIGMOID->1", 0,0);

/* Create a PSO trainer */
    pso = EncogTrainPSONew(30, net, data);
	EncogErrorCheck();

/* Begin training, report progress. */
    TrainNetwork(pso, 0.01f, 0);

/* Pull the best neural network that the PSO found */
    EncogTrainPSOImportBest(pso,net);
    EncogTrainPSODelete(pso);

/* Display the results from the neural network, see if it learned anything */
    printf("\nResults:\n");
    for(i=0; i<4; i++)
    {
        input = EncogDataGetInput(data,i);
        ideal = EncogDataGetIdeal(data,i);
        EncogNetworkCompute(net,input,output);
        *line = 0;
        EncogStrCatStr(line,"[",MAX_STR);
        EncogStrCatDouble(line,input[0],8,MAX_STR);
        EncogStrCatStr(line," ",MAX_STR);
        EncogStrCatDouble(line,input[1],8,MAX_STR);
        EncogStrCatStr(line,"] = ",MAX_STR);
        EncogStrCatDouble(line,output[0],8,MAX_STR);
        puts(line);
    }

/* Obtain the SSE error, display it */
    error = EncogErrorSSE(net, data);
    *line = 0;
    EncogStrCatStr(line,"Error: ",MAX_STR);
    EncogStrCatDouble(line,(double)error,4,MAX_STR);
    puts(line);

/* Delete the neural network */
    EncogNetworkDelete(net);
	EncogErrorCheck();

}

void train(char *egFile, char *egbFile) {
	ENCOG_DATA *data;
	ENCOG_NEURAL_NETWORK *net;
	ENCOG_TRAIN_PSO *pso;

	if( *egFile==0 || *egbFile==0 ) {
		printf("Usage: train [egFile] [egbFile]\n");
		return;
	}

	data = EncogDataEGBLoad(egbFile);
	EncogErrorCheck();

	printf("Training\n");
	printf("Input Count: %i\n", data->inputCount);
	printf("Ideal Count: %i\n", data->idealCount);
	printf("Record Count: %ld\n", data->recordCount);	    

	net = EncogNetworkLoad(egFile);
	EncogErrorCheck();

	if( data->inputCount != net->inputCount ) {
		EncogNetworkDelete(net);
		EncogErrorCheck();
		printf("Error: The network has a different input count than the training data.\n");
		return;
	}

	if( data->idealCount != net->outputCount ) {
		EncogNetworkDelete(net);
		EncogErrorCheck();
		printf("Error: The network has a different output count than the training data.\n");
		return;
	}

/* Create a PSO trainer */
	printf("Please wait...creating particles.\n");
    pso = EncogTrainPSONew(30, net, data);
	EncogErrorCheck();

/* Begin training, report progress. */	
    TrainNetwork(pso, 0.01f, 1);
	
/* Pull the best neural network that the PSO found */
    EncogTrainPSOImportBest(pso,net);
	EncogErrorCheck();

	EncogNetworkSave(egFile,net);
	EncogErrorCheck();

    EncogTrainPSODelete(pso);
	EncogErrorCheck();

	EncogNetworkDelete(net);
	EncogErrorCheck();
}

void EGB2CSV(char *egbFile, char *csvFile) 
{
	ENCOG_DATA *data;

	data = EncogDataEGBLoad(egbFile);
	EncogErrorCheck();

	printf("Converting EGB to CSV\n");
	printf("Input Count: %i\n", data->inputCount);
	printf("Ideal Count: %i\n", data->idealCount);
	printf("Record Count: %ld\n", data->recordCount);
	printf("Source File: %s\n", egbFile );
	printf("Target File: %s\n", csvFile );

	EncogDataCSVSave(csvFile,data,10);
	EncogErrorCheck();
	EncogDataDelete(data);
	EncogErrorCheck();

	printf("Conversion done.\n");
}

void CSV2EGB(char *csvFile, char *egbFile, int inputCount, int idealCount) 
{
	ENCOG_DATA *data;

	if( inputCount==-1 || idealCount==-1 ) {
		printf("You must specify both input and ideal counts.\n");
		exit(1);
	}

	data = EncogDataCSVLoad(csvFile, inputCount, idealCount);
	EncogErrorCheck();

	printf("Converting CSV to EGB\n");
	printf("Input Count: %i\n", data->inputCount);
	printf("Ideal Count: %i\n", data->idealCount);
	printf("Record Count: %ld\n", data->recordCount);
	printf("Source File: %s\n", csvFile );
	printf("Target File: %s\n", egbFile );

	EncogDataEGBSave(egbFile,data);
	EncogErrorCheck();
	EncogDataDelete(data);
	EncogErrorCheck();

	printf("Conversion done.\n");
}

void CreateNetwork(char *egFile, char *method, char *architecture, int inputCount, int idealCount) {
	ENCOG_NEURAL_NETWORK *network;

	if( *egFile==0 || *method==0 || *architecture==0 ) {
		printf("Must call with: create [egFile] [method] [architecture]\nNote: Because architecture includes < and > it must be includes in qutoes on most OS's.\n");
	}

	printf("Creating neural network\n");
	printf("Method: %s\n", method);
	printf("Architecture: %s\n", architecture);

	network = EncogNetworkFactory(method, architecture, inputCount,idealCount);
	EncogErrorCheck();
	printf("Input Count: %i\n",network->inputCount);
	printf("Output Count: %i\n",network->outputCount);

	EncogNetworkSave(egFile,network);
	EncogErrorCheck();
	printf("Network Saved\n");
}

void EvaluateError(char *egFile, char *egbFile) {
	ENCOG_DATA *data;
	ENCOG_NEURAL_NETWORK *net;
	ENCOG_TRAIN_PSO *pso;
	float error;
	char line[MAX_STR];

	if( *egFile==0 || *egbFile==0 ) {
		printf("Usage: error [egFile] [egbFile]\n");
		return;
	}

	data = EncogDataEGBLoad(egbFile);
	EncogErrorCheck();

	printf("Evaluate Error\n");
	printf("Input Count: %i\n", data->inputCount);
	printf("Ideal Count: %i\n", data->idealCount);
	printf("Record Count: %ld\n", data->recordCount);	    

	net = EncogNetworkLoad(egFile);
	EncogErrorCheck();

	if( data->inputCount != net->inputCount ) {
		EncogNetworkDelete(net);
		EncogErrorCheck();
		printf("Error: The network has a different input count than the training data.\n");
		return;
	}

	if( data->idealCount != net->outputCount ) {
		EncogNetworkDelete(net);
		EncogErrorCheck();
		printf("Error: The network has a different output count than the training data.\n");
		return;
	}

	error = EncogErrorSSE(net,data);

	EncogNetworkDelete(net);
	EncogDataDelete(data);

	*line = 0;
	EncogStrCatStr(line,"SSE Error: ",MAX_STR);
	EncogStrCatDouble(line,error*100.0,2,MAX_STR);
	EncogStrCatStr(line,"%\n",MAX_STR);
	puts(line);
}

void RandomizeNetwork(char *egFile) {
	ENCOG_NEURAL_NETWORK *net;

	if( *egFile==0  ) {
		printf("Usage: randomize [egFile]\n");
		return;
	}

	net = EncogNetworkLoad(egFile);
	EncogErrorCheck();

	EncogNetworkRandomizeRange(net,-1,1);
	EncogErrorCheck();

	EncogNetworkSave(egFile,net);
	EncogErrorCheck();


	printf("Network randomized and saved.\n");
}

int main(int argc, char* argv[])
{
	INT i;
	INT inputCount = -1;
	INT idealCount = -1;
	INT records = -1;
	INT iterations = -1;
	INT threads = 0;
	INT phase = 0;
	char command[MAX_STR];
	char arg1[MAX_STR];
	char arg2[MAX_STR];
	char arg3[MAX_STR];
	char *cudastr;
	
#ifdef ENCOG_CUDA
	cudastr = ", CUDA";
#else 
	cudastr = "";
#endif

	printf("\n* * Encog C/C++(%i bit%s) Command Line v0.1 * *\n",(int)(sizeof(void*)*8),cudastr);
	printf("Processor/Core Count: %i\n", (int)omp_get_num_procs());


	*arg1=*arg2=0;

	for(i=1;i<(INT)argc;i++) {
		if( *argv[i]=='/' || *argv[i]=='-' )
		{
			ParseOption(argv[i]);
			if( !EncogUtilStrcmpi(parsedOption,"INPUT") ) {
				inputCount = atoi(parsedArgument);
			} else if( !EncogUtilStrcmpi(parsedOption,"IDEAL") ) {
				idealCount = atoi(parsedArgument);
			} else if( !EncogUtilStrcmpi(parsedOption,"RECORDS") ) {
				records = atoi(parsedArgument);
			} else if( !EncogUtilStrcmpi(parsedOption,"ITERATIONS") ) {
				iterations = atoi(parsedArgument);
			} else if( !EncogUtilStrcmpi(parsedOption,"THREADS") ) {
				threads = atoi(parsedArgument);
				omp_set_num_threads(threads);
			}
			
		}
		else 
		{
			if( phase==0 ) {
				strncpy(command,argv[i],MAX_STR);
				EncogUtilStrlwr(command);
			} else if( phase==1 ) {
				strncpy(arg1,argv[i],MAX_STR);
			} else if( phase==2 ) {
				strncpy(arg2,argv[i],MAX_STR);
			} else if( phase==3 ) {
				strncpy(arg3,argv[i],MAX_STR);
			}

			phase++;
		}
	}
	
	EncogUtilInitRandom();
	
	if(!EncogUtilStrcmpi(command,"xor") ) {
		XORTest();
	} else if (!EncogUtilStrcmpi(command,"benchmark") ) {
		RunBenchmark(inputCount,idealCount,records,iterations );
	} else if (!EncogUtilStrcmpi(command,"train") ) {
		train(arg1,arg2);
	} else if (!EncogUtilStrcmpi(command,"egb2csv") ) {
		EGB2CSV(arg1,arg2);
	} else if (!EncogUtilStrcmpi(command,"csv2egb") ) {
		CSV2EGB(arg1,arg2,inputCount,idealCount);
	} else if (!EncogUtilStrcmpi(command,"create") ) {
		CreateNetwork(arg1,arg2,arg3,inputCount,idealCount);
	} else if (!EncogUtilStrcmpi(command,"randomize") ) {
		RandomizeNetwork(arg1);
	} else if (!EncogUtilStrcmpi(command,"error") ) {
		EvaluateError(arg1,arg2);
	} else if (!EncogUtilStrcmpi(command,"cuda") ) {
#ifdef ENCOG_CUDA
		TestCUDA();
#else
		printf("CUDA is not available in this distribution od Encog\n");
		printf("If you wish to use CUDA, please download a CUDA enabled version of Encog.\n");
#endif
	} else {
		Usage();
	}


    return 0;
}