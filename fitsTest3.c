//to compile: gcc -o fitsTest fitsTest.c -lcfitsio
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
        char *p;
        FILE *in,*out,*input,*output,*mainOut, *finalOut;
        char *batLine, *sentbatch, *outputLine, *command, *finalCommand, *lOut, *spec, *outEnd;
        char *namechar, *signal;
        int currentTask,numTasks, rank, i,j,loopCount = 0, error,w,w2, lines, count = 0, tempv;
        const char letterm = 'm',letterf = 'f', period = '.';
        const char reject[80] = "Spectrum was rejected";
        bool *Continue = (bool*)malloc(sizeof(bool));
        bool exitFlag = false,noSend = false, sentOnce = false, done, name, final,first;
        fitsfile *fitsFile;
		int fitsstatus = 0; //has to be initilized
        float RA, DEC;

        strcpy(dirfile,argv[1]);
        p = strrchr(dirfile,'.');
        n = p-dirfile;
        strncpy(Dirname,dirfile,n);
        Dirname[n] = '\0';
        chdir(Dirname);
		
		
		
        mainOut = fopen("lamost.out", "r");
        sprintf(buffer, "%s.out", Dirname);
        finalOut = fopen(buffer,"w+");
        outputLine = (char*)calloc(90,sizeof(char));
		namechar = (char*)calloc(15, sizeof(char));
        signal = (char*)calloc(10, sizeof(char));
        spec = (char*)calloc(50, sizeof(char));
        outEnd = (char*)calloc(50, sizeof(char));
        int strlength =  0;
        while(fgets(outputLine,90,mainOut) != NULL)
        {
                strlength = 0;
                //takes the place of calling calloc and free each time to erase contents
                //as doing that causes malloc invalid size errors for no real reason.
                if(sentOnce)
                {
                        for(i = 0; i<50;i++)
						spec[i] = NULL;
                }
                done = false; name = false; final = false, first = true;
                j = 0; w =0;
                //goes through line to get spectrum name and saves the rest to outEnd
                for(i =0; i<90;i++)
                {
                        if(outputLine[i] == period && name == false)
                        {
                                printf("spec b4 file:%s\n",spec);
                                fprintf(finalOut,"%s.nor |",spec);
                                spec[i] = outputLine[i];
                                done = true;
								tempv = i;
                                strlength++;
                        }
                        else if(done && i == tempv + 1)
                                spec[i] = 'f';
                        else if(done && i == tempv + 2)
                                spec[i] = 'i';
                        else if(done && i == tempv + 3)
                                spec[i] = 't';
                        else if(done && i == tempv + 4)
                                spec[i] = 's';
                        else if(done && i == tempv + 5)
                        {
								strlength = strlength + 7;
                                printf("file:%s\n",spec);
                                name = true; //done getting name
                        }
                        else if(!done)
                        {
                                spec[i] = outputLine[i];
                                strlength++;
                        }
                        //need to store the rest of the data
                        if(i == strlength   && name==true && done == true)
                        {
                                outEnd[j] = outputLine[i];
								j++; w++;
                                printf("in strlen\n");
                                final = true;
                                if(j ==49)
                                {
                                        printf("j too big1\n");
                                        break;
                                }
                        }
                        else if(final == true)
                        {
                                outEnd[j] = outputLine[i];
                                j++;
//can't remove extra spaces after classification
                                //as they aren't actually spaces and trying to limit it doesn't work
                        }
                }
                printf("%s\n", outEnd);
                //get data from fits file
                fits_open_file(&fitsFile,spec,READONLY,&fitsstatus);
                fits_get_errstatus(fitsstatus,buffer);
                printf("open error:%s\n",buffer);

                fits_read_key(fitsFile,TFLOAT,"RA_OBS",&RA,NULL,&fitsstatus);
                fits_get_errstatus(fitsstatus, buffer);
                printf("RA error: %s  | ",buffer); //works
				printf("RA=%f\n",RA);
                 //RA should be in hours instead of degrees.  To do that, divide RA in degrees by 15.0.
                RA = RA / 15.0; //puts RA in hours
                fits_read_key(fitsFile,TFLOAT,"DEC_OBS",&DEC,NULL,&fitsstatus); //works
                fits_get_errstatus(fitsstatus, buffer);
                //RA should be in hours instead of degrees.  To do that, divide RA in degrees by 15.0.
                printf("DEC error: %s  | ",buffer);
                printf("DEC=%f\n",DEC);
                fits_read_key(fitsFile,TSTRING,"OBJNAME",namechar, NULL, &fitsstatus);
                fits_get_errstatus(fitsstatus, buffer);
                printf("OBJ error: %s  | ",buffer);
                printf("OBJ: %s\n",namechar);
                fits_read_key(fitsFile,TSTRING,"SNRG", signal, NULL, &fitsstatus);
 fits_get_errstatus(fitsstatus, buffer);
                printf("SNRG error: %s  | ",buffer);
                printf("SNRG:%s\n",signal);
                fits_close_file(fitsFile,&fitsstatus);
                printf("strlength: %d \n",strlen(signal));
                if(strlen(signal) < 6)
                {
                        signal[5] = ' ';
                        if(strlen(signal) < 5)
                        signal[4] = ' ';
                }
                fprintf(finalOut," %f  %f | %s | %s | %s",RA,DEC,namechar,signal,outEnd);
                printf("wrote to file\n");
				sentOnce = true;
        }

        printf("freeins stuff\n");
        free(outputLine);
        free(signal);
        free(namechar);
        free(spec);
        free(outEnd);
        fclose(finalOut);
        //cannot close mainOut or will give a free error
//      fclose(mainOut);
        printf("freed\n");
 //rename file to proper name (tar file name.out) and
        //send output to /media/pi/DR5/results
        sprintf(buffer, "cp ./%s.out /media/pi/LAMOST/Results/%s.out", Dirname, Dirname);
        err = system(buffer);
        return(0);
}
