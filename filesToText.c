#include <stdio.h> //needed for file in/outp
#include <dirent.h> //interface for format of directories
				//defines DIR a type to represent directory stream
				//defines structure dirent that includes
					//ino_t d_ino a file serial number
					//char d_name[] filename string of ent
#include <sys/types.h> 	//defines the ino_t type (used for file serial numbers)					
#include <sys/stat.h> //uses types defined by types.h and has useful functions
#include <stdlib.h> 
#include <string.h>
int main(int argc, char *argv[])
{
	struct dirent *direntPointer;
	DIR *dirPointer; //directory pointer
	FILE *output;
	
	//error handling
	if(argc != 2)
	{
		//only want one directory to be passed in
		printf("Usage: getAllFiles then name of one directory\n");
		exit(1); //exit with 1 indicates a failure
	}
	dirPointer = opendir(argv[1]); //gives dfd pointer to directory passed in
	if(dirPointer == NULL)
	{
		printf("Could not open %s", argv[1]);
		exit(1);
	}
	//Finding number of files in the given directory 
	int fileNumber = 0;
	//readdir returns null when it has reached the end and when encounters an error
	//readdir returns pointer to a struct dirent when succesful
	while((direntPointer = readdir(dirPointer)) != NULL)
	{
		if(direntPointer -> d_type == DT_REG)
		{
			fileNumber ++; //only want to count.tar files
		}	
	}
	printf("Number of files in %s : %d \n", argv[1],fileNumber);
	printf("Known number of files in DR5: 3956 \n");
	closedir(dirPointer);
	dirPointer = opendir(argv[1]);
	output = fopen("fileNames.txt", "w+");
	unsigned int i = 0;
	
	while((direntPointer = readdir(dirPointer)) != NULL)
	{
		if(direntPointer -> d_type == DT_REG)
		{
			fprintf(output,"%s %s",direntPointer -> d_name, ",");
			//fprintf(direntPointer -> d_name[i]);
		}	
	}
	closedir(dirPointer);
	fclose(output);		
	return(0);
	
	//testing file read (not part of actual needed code)
	FILE *input;
	input = fopen("partialNames.txt","r");
	char fileName[50];
	while(1)
	{
		//fgets returns null at end of file or on error
		if(fgets(fileName,50,input) != NULL)
		{
			printf("%s \n",fileName);
		}
		else
		{
			break;
		}
	}
	fclose(input);	
}

