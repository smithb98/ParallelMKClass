//New version for run_mkclass_pi that will use MPI
//to handle parrellel processing
//run_mkclass_pirallel
//Version: 4, 06/18/2020
//Author: Brittany Smith
//Improved version of run_mkclass_pi that can run across all 4 nodes
//with each node having one task per cpu core
//Must be on all nodes at ~/progs
//version 3 adds in code to add information from the .fits file to the output files
//MUST COMPILE IT ON EACH NODE OR CFITSIO WILL MESS UP
//mpicc -o shormkclass_pirallel runversion4.c -lcfitsio needs to be done seperatly on each node
//Version 4 adds in functionality for any number of tasks and hosts and requires 
//2 command line arguments to know the number of hosts
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mpi.h"
#include <stdio.h>
#include <stdbool.h>
#include "fitsio.h"
#include <sys/types.h>
#include <sys/wait.h>
int main(int argc, char *argv[])
{
	char dirname[80],buffer[80],filename[80],tempname[80],extractname[80];
	char Dirname[80],dirfile[80];
	int err,n;
	FILE *in,*out,*input,*output,*mainOut, *finalOut;
	char *batLine, *sentbatch, *outputLine, *command, *finalCommand, *lOut, *spec, *outEnd;
	char *namechar, *signal, *p;
	int currentTask,numTasks, rank, i,j,loopCount = 0, error,w, lines, count = 0, tempv, hosts, taskPerHost;
	const char letterm = 'm',letterf = 'f', period = '.';
	const char reject[80] = "Spectrum was rejected";
	bool *Continue = (bool*)malloc(sizeof(bool));
    MPI_Status stat, stat2;
	bool exitFlag = false,noSend = false, sentOnce = false, done, name, final, first;
	fitsfile *fitsFile;
	int fitsstatus = 0; //has to be initilized
	float RA, DEC;

	//MPI additions
	MPI_Init(&argc,&argv); 	//initilize mpi enviroment
	MPI_Comm_size(MPI_COMM_WORLD, &numTasks); //finds total tasks 
	MPI_Comm_rank(MPI_COMM_WORLD,&rank); //assigns a rank for each task
  
	if(argc != 3) 
	{
		printf("Usage: run_mkclass number of hosts dirname.tar\n");
		printf("Should be using start_mkclass_pirallel to run this version\n");
		exit(1);
	}
	strcpy(dirfile,argv[2]);
	p = strrchr(dirfile,'.');
	n = p-dirfile;
	strncpy(Dirname,dirfile,n);
	Dirname[n] = '\0';
	hosts = atoi(argv[1]); //passing in number of hosts as 3rd command line argument
	taskPerHost = numTasks / hosts; 
	if(rank == 0)
	{
		sprintf(buffer, "cp -r /media/pi/LAMOST/DR5/%s ./", dirfile);
		err = system(buffer);
	}
	//only works for 16 tasks over 4 hosts:
	//else if(rank == taskPerHost || rank == taskPerHost *2|| rank == taskPerHost *3)				
		//(rank == 4 || rank == 8 || rank == 12)
	else
	{
		for( i = 1; i < hosts; i++)
		{
			if(rank == taskPerHost * i)
			{
				sprintf(buffer,"scp -r pi@10.0.0.3:/media/pi/LAMOST/DR5/%s ./",dirfile);
				err = system(buffer);
			}
		}
		
	}
	//steps 1-6 only need to occur once per node
	for(i = 0; i <hosts; i++)
	{
		if(rank == taskPerHost*i)
		{
			sprintf(buffer,"tar -xvf %s",dirfile);
			err = system(buffer);
			//changes directory to be ~progs/tarname
			chdir(Dirname);

			err = system("gunzip *.gz");

			system("ls *.fits > temp.txt");

			in = fopen("temp.txt","r");
			if(in == NULL) 
			{
				printf("Cannot open temp.txt\n");
				exit(1);
			}
			out = fopen("extract.bat","w");
			//4) uses that list to make a batch file, extract.bat, which extracts the 
			//data in the fits file to a text file readable by MKCLASS
			while(fscanf(in,"%s",filename) != EOF) 
			{
				fprintf(out,"extractLAMOST2018 %s\n",filename);
			}
			fclose(in);
			fclose(out);
		
			err = system("chmod a+x extract.bat");
			//5 runs that batch
			err = system("./extract.bat");
			//6 runs the program mkLMSTbat, which makes a
			// batch file for running MKCLASS on all the spectra.
			//file is named lamost.bat and is within current directory
			err = system("mkLMSTbat");
		}
	}
	if (rank ==0)
	{
		
		MPI_Barrier(MPI_COMM_WORLD); //need barrier to prevent other tasks from
			//trying to do things before task 0 is ready
		printf("in mpi part rank0\n");
		input = fopen("lamost.bat","r");
		mainOut = fopen("lamost.out", "w+");
		lines = 0;
		
		while(1)
		{
			count = 1; //reset counter
			//test limit if here 
			if(exitFlag)
				break; //allows to break out of while and for when no more lines exist
			command = (char*)calloc(105,sizeof(char));
			finalCommand = (char*)calloc(110,sizeof(char));
			batLine = (char*)calloc(105,sizeof(char)); //allocate memory for batLine array
			lOut = (char*)calloc(85,sizeof(char));
			sentOnce = false; //haven't sent bcast yet
			for(i=0; i < numTasks; i++) 
			{			
				//fgets needs to be character count of longest line + 2 or it fails
				if(fgets(batLine,105,input) != NULL)
				{		
					if(loopCount> 0 && sentOnce == false)
					{
						*Continue = true;
						sentOnce = true;
						//MPI_Bcast ( void *buffer, int count, MPI_Datatype datatype, int root,
						// MPI_Comm comm )
						MPI_Bcast(Continue,1,MPI_C_BOOL,0,MPI_COMM_WORLD);
					}
					if(i==0)
					{
						strcpy(command,batLine); //to save batline data
					}
					else
					{
						//MPI_Send(buffer,count,type,dest,tag,comm)
						MPI_Send(batLine,105,MPI_CHAR,i,i,MPI_COMM_WORLD); 
						printf("sent rank%d: %s\n",i,batLine);
						fflush(stdout);
						count++;
					}
				}
				else
				{
					*Continue = false;
					exitFlag = true; //flag to break out of while loop
					free(batLine);
					batLine = (char*)calloc(105,sizeof(char));
					batLine[0]='e';
					MPI_Send(batLine,105,MPI_CHAR,i,i,MPI_COMM_WORLD); 
			
				}		
			}	
			//free memory used by batline when it is not being used
			free(batLine);
			bool check = false;
				int location = 0;
				char temp[1]; 
				for(i =0; i< 110; i++)
				{
					finalCommand[i] = command[i];
					if(i == 10)
					{
						finalCommand[i] = letterm;
					}
					if(i > 10)
					{
						finalCommand[i] = command[i-1];
											
						temp[0] = finalCommand[i];
						if(finalCommand[i] == letterf && i>60)	///so when have reached f
						{
							location = i;
							check = true; 
							finalCommand[i] = '0';
						}
						else if(check && (i - 1) == location )
						{
							finalCommand[i] = ' ';
						}
						else if(check && (i-2) == location)
						{
							finalCommand[i] = letterf;
						}
					}
					
				}
			error = system(finalCommand); 	///to run batch file line
			MPI_Barrier(MPI_COMM_WORLD); //to ensure every task is done processing given batch line
			//to ensure the proper order, need tomake sure tasks 1-15 send it in right order
			free(command);
			free(finalCommand);
			if (error != NULL) //gives a compile warning but works 
			{
				//do nothing
			}
			else
			{			
				lines++;
				output = fopen("lamost0.out","r"); 
				for(i =1; i <= lines; i++)
				{
					fgets(lOut,85,output); //gets data from file	
				}
				fclose(output);
				fprintf(mainOut,"%s", lOut); //adding data from rank0 first
			}
			outputLine = (char*)calloc(85,sizeof(char)); 
			free(lOut);
			for (i=1; i<count;i++) //task 0 doesn't send data. have to start at 1
			{
				//MPI_Recv(buffer,count,type,source,tag,comm,status)
				MPI_Recv(outputLine,85,MPI_CHAR,i,numTasks,MPI_COMM_WORLD,&stat2);
				//will use tag as numtasks for sending data back to task 0
				//don't want to add in the rejected message
				if(strncmp(outputLine,reject,20) != 0)	
					fprintf(mainOut,"%s",outputLine);
			}
			MPI_Barrier(MPI_COMM_WORLD);
			free(outputLine);
			loopCount ++;
			if(exitFlag)
			{
				Continue = false;
				MPI_Bcast(Continue,1,MPI_C_BOOL,0,MPI_COMM_WORLD);
				break;
			}
			 //to ensure proper exit on 16 lines or 16*n lines
            batLine = (char*)calloc(105,sizeof(char)); //need batline again
            fpos_t pos; //to save current file index position
            fgetpos(input, &pos);//saving it
            if(fgets(batLine,105,input)==NULL)
            {
                *Continue = false;
                MPI_Bcast(Continue,1,MPI_C_BOOL,0,MPI_COMM_WORLD);
                free(batLine);
                break;
            }
            fsetpos(input, &pos);   ///resets position in case of more lines

		//only task0 can do anything with the main files
		fclose(input);
		fclose(mainOut);
       }
		
   else 
    {
		MPI_Barrier(MPI_COMM_WORLD);
		//just puts all output files in same place to make it easy to delete
		for(i = 0; i< hosts; i++)
		{
			if(rank!= taskPerHost * i)
				chdir(Dirname);
		}
		lines = 0;
        while(1)
		{
			finalCommand = (char*)calloc(110,sizeof(char));
			lOut = (char*)calloc(85,sizeof(char));
			sentbatch = (char*)calloc(105,sizeof(char));	
			//MPI_Recv(buffer,count,type,source,tag,comm,status)
			//using rank as flag makes it so only the wanted rank gets sent the data
			MPI_Recv(sentbatch,105,MPI_CHAR,0,rank,MPI_COMM_WORLD,&stat);
			if(strncmp(sentbatch,"e",1) == 0)
			{
				noSend = true; 
				//don't want to send back the place holder data
				free(sentbatch);
			}
			else
			{
				bool check = false;
				int location = 0;
				char temp[2]; 
				sprintf(temp,"%d",rank);
				for(i =0; i< 110; i++)
				{
					finalCommand[i] = sentbatch[i];
					if(i == 10)
					{
						finalCommand[i] = letterm;
					}
					if(i > 10)
					{
						finalCommand[i] = sentbatch[i-1];
						if(rank < 10)
						{
							if(finalCommand[i] == letterf && i>60)	///so when have reached f
							{
								location = i;
								check = true;
								finalCommand[i] = temp[0];
							}
							else if(check && (i - 1) == location )
							{
								finalCommand[i] = ' ';
							}
							else if(check && (i-2) == location)
							{
								finalCommand[i] = letterf;
							}
						}
						else 
						{
							if(finalCommand[i] == letterf && i>60)	///so when have reached f
							{
								location = i;
								check = true;
								finalCommand[i] = temp[0];
							}
							else if(check && (i-1) == location)
							{
								finalCommand[i] = temp[1];
							}
							else if(check && (i - 2) == location )
							{
								finalCommand[i] = ' ';
							}
							else if(check && (i-3) == location)
							{
								finalCommand[i] = letterf;
							}
						}
					}
				}
				free(sentbatch); //don't want to waste memory space 
				printf("%s\n",finalCommand); //prints out fine
				error = system(finalCommand); //should run batch line
				free(finalCommand);
				if (error != NULL) //gives a compile warning but works 
				{
					//dont do other stuff just copy in message so it is sent to rank 0
					strcpy(lOut,reject);
				}
				else
				{
					//files save as lamostn.out 
					lines++;
					sprintf(buffer, "lamost%d.out",rank);
					output = fopen(buffer,"r"); 
					//since we need to close the file and reopen it each time
					//need to read until at proper line
					for(i =1; i <= lines; i++)
					{
						fgets(lOut,85,output); //gets data from file
						
					}
					fclose(output);
				}
			}
			//MPI_Barrier -  Blocks until all process have reached this routine.
			//Blocks  the  caller  until  all  group members have called it; the call
			//returns at any process only after all group members  have  entered  the call
			MPI_Barrier(MPI_COMM_WORLD);
			if(noSend == false)
			{
				MPI_Send(lOut,85,MPI_CHAR,0,numTasks,MPI_COMM_WORLD);
			}
			free(lOut);
			free(Continue);
			Continue = (bool*)malloc(sizeof(bool));
			MPI_Barrier(MPI_COMM_WORLD);
			//MPI_Bcast ( void *buffer, int count, MPI_Datatype datatype, int root,
						// MPI_Comm comm )
			//both the sender and the recievers need to call bcast
			MPI_Bcast(Continue,1,MPI_C_BOOL,0,MPI_COMM_WORLD);
			if(*Continue == 0)
				break;	
		}
	}
	//to remove extra lamost files
	sprintf(buffer, "rm lamost%d.out",rank);
	system(buffer);
	for (i = 0; i< hosts; i++)
	{
		if(rank == taskPerHost*i)
		{
			err = system("rm *.fits");
			err = system("rm *.flx");
			err = system("rm *.nor");
			err = system("rm *.mat");
			err = system("rm *.cor");
			err = system("rm *.gz");
		}
	}
	MPI_Finalize(); //required after done with MPI
	return(0);
}


