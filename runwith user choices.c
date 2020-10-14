//start_mkclass_pirallel
//Author: Brittany Smith
//Takes one tar file from fileNames.txt at at time and uses it to
//start up run_mkclass_pirallel with a user specified amount of tasks and hosts
//user will be asked to input needed information on command line after starting the code
//but it will only need to be entered once
//If the length of the tar file names ever exceeds 100, increase constant MAXFILENAMELENGTH
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
int checkHosts(int numberHosts, int avHosts,int userLength);
#define TRUE 1
#define FALSE 0

char userInput[100], buffer[60], tempname[100];
char *hosts, *tempHost, *program, *fullProgram;
FILE *nmap;
int maxhost;
int main(int argc, char *argv[])
{
    const int MAXFILENAMELENGTH = 100;
	FILE *input;
    char tarFile[100]; //4X the length of longest name
	//mpiexec -N 4 --host 10.0.0.3,10.0.0.4,10.0.0.5,10.0.0.6 -oversubscribe run_mkclass_pirallel tarnamehere        
	const char programName[94] = "mpiexec -N 4 --host 10.0.0.3,10.0.0.4,10.0.0.5,10.0.0.6 -oversubscribe run_mkclass_pirallel ";
 //   char userInput[100], buffer[60], tempname[100];
//	char *hosts, *tempHost, *program, *fullProgram;
	int length, i, j = 0, totLength = 58, tasks, tarLen, numHosts = 0; //regardless of user input min chars will be 55 
	char answer;
	bool incorrect = true, firstHost = true;
	int availableHosts = 0, maxHostLength = 30; //longest an available host name can be
	if(argc != 2)
    {
        //only want one directory to be passed in
        printf("Usage: startMkClassParallel tarfilenames.txt\n");
		printf("Run filesToText to create a usable text file called fileNames.txt\n");
        exit(1); //exit with 1 indicates a failure
    }
	
	printf("Enter the host names to run on using the following format:\n");
	printf("host1,host2,host3; or 10.0.0.3,10.0.0.4,10.0.0.5;\n");
	printf("Where the first host listed will have the master task\n");
	scanf("%s", userInput);
	printf("Enter max length for host names\n");
    scanf("%d",&maxhost);
	length = strlen(userInput);
	totLength += length;
	hosts = (char*)calloc(length,sizeof(char));
	tempHost = (char*)calloc(20,sizeof(char));
	int templength = 0;
	for(i = 0;i < length;i++)
	{
		if(userInput[i] == ',')
		{
			//check validity of tempHost here
			if(firstHost)
			{
				sprintf(buffer,"nmap -sn %s/24 --open -oN nmap.txt",tempHost);
				system(buffer);
				nmap = fopen("nmap.txt", "r");
				while(fgets(tempname,100,nmap) != NULL)
				{
					if(tempname[0] == '#')
						availableHosts = availableHosts; //aka do nothing
					else if(tempname[0] == 'H')
						availableHosts = availableHosts; //may want to check availbility later
					else if(tempname[0] == 'N')
					{
						//Nmap scan report for 10.0.0.2
						//21 chars to start
						availableHosts ++;
					}
				}
				firstHost = false;
				fclose(nmap);
			}
			else
			{
				
			}
			//reset temp host in case host names are different lengths
			free(tempHost);
			tempHost = (char*)calloc(20,sizeof(char));
			j = 0;
			hosts[i] = userInput[i];
			numHosts ++;
			if(templength >maxhost)
				maxhost = templength;
			templength = 0;
		}
		else if (userInput[i] == ';')
		{
			numHosts++;
			if(templength >maxhost)
				maxhost = templength;
			//done gettng hosts, but still need to check last host here
		}
		else
		{
			hosts[i] = userInput[i];
			tempHost[j] = userInput[i];
			j++; templength++;
		}
	}
	//want to hang up if it fails and reget the entered hosts
	while(checkHosts(numHosts,availableHosts,length) == FALSE)
	{
		free(hosts); //dont want to keep the old bad hostlist
		totLength = totLength - length; //want to remove the old length value
		printf("Enter the host names to run on using the following format:\n");
		printf("host1,host2,host3; or 10.0.0.3,10.0.0.4,10.0.0.5;\n");
		printf("Where the first host listed will have the master task\n");
		scanf("%s", userInput);
		printf("Enter max length for host names\n");
        scanf("%d",&maxhost);
		length = strlen(userInput);
		totLength += length;
		hosts = (char*)calloc(length,sizeof(char));
		tempHost = (char*)calloc(20,sizeof(char));
		for(i = 0;i < length;i++)
		{
			if(userInput[i] == ',')
			{
				//check validity of tempHost here
				if(firstHost)
				{
					sprintf(buffer,"nmap -sn %s/24 --open -oN nmap.txt",tempHost);
					system(buffer);
					nmap = fopen("nmap.txt", "r");
					while(fgets(tempname,100,nmap) != NULL)
					{
						if(tempname[0] == '#')
							availableHosts = availableHosts; //aka do nothing
						else if(tempname[0] == 'H')
							availableHosts = availableHosts; //may want to check availbility later
						else if(tempname[0] == 'N')
						{
							availableHosts ++;
						}
					}
					firstHost = false;
					fclose(nmap);
				}
				else
				{
				
				}
				//reset temp host in case host names are different lengths
				free(tempHost);
				tempHost = (char*)calloc(20,sizeof(char));
				j = 0;
				hosts[i] = userInput[i];
				numHosts ++;
			}
			else if (userInput[i] == ';')
			{
				numHosts++;
				//done gettng hosts, but still need to check last host here
			}
			else
			{
				hosts[i] = userInput[i];
				tempHost[j] = userInput[i];
				j++;
			}
		}
	}
	
///can run nmap -sn host1/24 and it will give other hosts available in command lines
//need to find a way to get that output and use it to check the rest of the hosts
	printf("Enter the number of tasks to run on each host\n");
	scanf("%d",&tasks);
	printf("Running more tasks than available processors may result in unexpected behavoir\n");
	printf("Are you sure you want to run %d tasks per host? Y/N \n",tasks);
	while(incorrect)
	{
		scanf("%c",&answer);
		switch(answer)
		{
			case 'y':
			case 'Y':
				incorrect = false;
				break;
			case 'n':
			case 'N':
				printf("exiting now\n");
				return(0);
			default:
				printf("Enter Y or N only\n");
				break;
		}
	}
	if(tasks >99)
		totLength +=3; //shouldn't be needed but the option will work
	else if(tasks > 9)
		totLength+= 2;
	else
		totLength++;

	if(numHosts>999)
		totLength+=4;
	else if(numHosts >99)
		totLength +=3;
	else if(numHosts >9)
		totLength +=2;
	else 
		totLength++;
	//"mpiexec -N 4 --host 10.0.0.3,10.0.0.4,10.0.0.5,10.0.0.6 -oversubscribe run_mkclass_pirallel "
	program = (char*)calloc(totLength, sizeof(char));
	sprintf(program, "mpiexec -N %d --host %s -oversubscribe run_mkclass_pirallel",tasks,hosts);
    input = fopen(argv[1],"r");
	//Needs to occurs each time the curent tar file is done proccessing
    if( fgets(tarFile,MAXFILENAMELENGTH,input) != NULL)
    {
        //first iteration only:
		tarLen = strlen(tarFile); //should get length of current elements only
        printf("tar file:%s\n", tarFile);
		tarLen+= totLength; //tarlen is also used as full program length
		fullProgram = (char*)calloc(tarLen, sizeof(char));
		sprintf(fullProgram,"%s %d %s",program,numHosts,tarFile); //combines everything
        int waitStatus = system(fullProgram);
		free(fullProgram); //removes all elements in fullprogram
		sprintf(buffer, "./extractFits %s", tarFile);
        waitStatus = system(buffer);

        while (WIFEXITED(waitStatus)) //returns true if system call returned normally or
        {
            //still only want it to continue only if the next tar exists
            if( fgets(tarFile,MAXFILENAMELENGTH,input) != NULL)
            {
				tarLen = strlen(tarFile); //should get length of current elements only
				printf("tar file:%s\n", tarFile);
				tarLen+= totLength; //tarlen is also used as full program length
				fullProgram = (char*)calloc(tarLen, sizeof(char));
				sprintf(fullProgram,"%s %d %s",program,numHosts,tarFile); //combines everything
				waitStatus = system(fullProgram);
				free(fullProgram);
				sprintf(buffer, "./extractFits %s", tarFile);
				waitStatus = system(buffer);

            }
            else
            {
                break; //ensures the while loop will be exited when reach end of file
            }
        }
    }
    else
    {
        printf("Reached end of all tar files given");
    }
	fclose(input);
    return(0);
}

int checkHosts(int numberHosts, int avHosts,int userLength)
{
	int i, j, max = maxhost + 1; //need to have '\0' at end of each string
	char fgetsdata
	
	//going through to put all hosts in there own data
	int width = userLength / numberHosts;
	char *givenHosts[numberHosts];
	//allocate givenhosts 2-d array
	for(i = 0; i < numberHosts;i++)
	{
		givenHosts[i] = (char*) malloc(max*sizeof(char));
	}
	//allocate arrays for available hosts
	char *available[avHosts];
	for(i = 0; i < avHosts;i++)
	{
		available[i] = (char*) malloc (max *sizeof(char));
	}
	//store data of possible hosts
	nmap = fopen("nmap.txt", "r");
	printf("data of av hosts\n");
	//have to manually increment i and use while(1) as using for loop
	//will allow i to increase even when no data is saved
	i = 0;
	while(1)
	{
		fgets(tempname,100,nmap);
		if(tempname[0] == 'N')
		{
			for(j= 0; j< maxHostLength; j++)
			{
				if(tempname[j+21] == '\0'||tempname[j+21] == ' ' || tempname[j+21] == '\n')
					break;
				else
				{	//Nmap scan report for 10.0.0.2
					//21 chars to start
					available[i][j] = tempname[j+21];
				}
			}
			if(!(fgetsdata[21+maxhost] <= 0x39 && fgetsdata[21+maxhost]<= 0x30 || fgetsdata[21+maxhost] == 0x2e))
			{
				
			}

			
			available[i][maxhost] = '\0';
			printf("%s\n",available[i]);
			i++;
			if(i >= avHosts)
				break;
		}
	}
	fclose(nmap);
	
	//need to check each host name to see if it is available
	int same, k;
bool valid = false;
tempHost = (char*)calloc(maxhost,sizeof(char));
j = 0;
for(i = 0;i < userLength;i++)
{
    if(userInput[i] == ',')
    {
        tempHost[i] = '\0';
        //check validity of tempHost here
        for(k = 0; k <avHosts; k++)
        {
            printf("av[%d]: %s\n",k,available[k]);
            same = strcmp(tempHost,available[k]);
            if(same == 0)
            {
                printf("%s is valid\n", tempHost);
                valid = true;
                break;
            }
        }
        if(valid == false)
        {
            printf("Host: %s is not available\n", tempHost);
			return(FALSE);
        }
        //reset temp host in case host names are different lengths
        free(tempHost);
        tempHost = (char*)calloc(maxhost,sizeof(char));
        j = 0;
    }
    else if (userInput[i] == ';')
    {
        tempHost[i] = '\0';
        for(k = 0; k <avHosts; k++)
        {
            same = strcmp(tempHost,available[k]);
			if(same == 0)
            {
                printf("%s is valid\n", tempHost);
                valid = true;
                break;
            }
        }
        if(valid == false)
        {
            printf("Host: %s is not available\n", tempHost);
            return(FALSE);
        }
    }
	else
    {
        tempHost[j] = hosts[i];
        j++;    
    }
	 //can only get here if valid was never false
    return(TRUE);	
}
