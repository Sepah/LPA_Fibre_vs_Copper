/*
*
* code structure---
* 
* Beautify code! :) 
* http://www.tutorialspoint.com/online_c_formatter.htm
*
* #####COMMANDS#####
* * COMPILE: gcc -Wall -o filename_out filename_in.c -lwiringPi -lm
* * EXECUTE sudo ./filename_out
* *
* * TERMINATE INFINITE LOOP: CTRL+C 
* 
* 
***********************************************************************
*/

#include <time.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#define LPA_FOCat 1.0
#define GPIO_pinoff 1
#define GPIO_pinon 0

int dump(char *filename);
int read_eeprom(unsigned char);
int ddm(void);

int control_on(void);
int control_off(void);
int control_break(void);

int read_sfp(void);
int xio;
unsigned char A50[256]; //Bytes 0 to 127 (serial ID and Vendor Specifc)
unsigned char A51[256]; //DDM Data - 96 to 105 (TX, RX, Temp, Vcc)

int main (int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "rmdIO :")) !=-1) 
    {
        switch (opt) 
        {
        case 'r':
            read_sfp();
            break;
        case 'm':
            ddm();
            break;
        case 'd':
            dump(optarg);
            break;
        case 'I':
            control_on();
            break;
        case 'O':
            control_off();
            break;

			
        default: /* '?' */
            fprintf(stderr,
                    "Usage: %s\n"
                    "	-r read the sfp transceiver\n"
                    "	-m Print DDM values if SFP supports DDM\n"
                    "	-d filename - dump the eprom to a file\n"
                    "	-I switch on\n"
                    "	-O switch off\n"
                    ,argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (argc <=1) {
        fprintf(stderr,
                    "Usage: %s\n"
                    "	-r read the sfp transeiver\n"
                    "	-m Print DDM values if SFP supports DDM\n"
                    "	-d filename - dump the eprom to a file\n"
                    "	-I switch on\n"
                    "	-O switch off\n"
                ,argv[0]);
        exit(EXIT_FAILURE);
    }
}
int read_sfp(void)
{	
	static char *identifier[17] =
	{
		"Unknown",
		"GBIC",
		"Module/connector soldered to motherboard",
		"SFP or SFP+",
		"Reserved for 300 pin XBI devices",
		"Reserved for XENPAK devices",
		"Reserved for XFP devices",
		"Reserved for XFF devices",
		"Reserved for XFP-E devices",
		"Reserved for XPAX devices",
		"Reserved for X2 devices",
		"Reserved for DWDM-SFP devices",
		"Reserved for QSFP devices",
		"Reserved for QSFP+ devices",
		"Reserved for CXP devices",
		"Unallocated",
		"Vendor specific"
	};//1 byte - address 0
    unsigned char vendor[16+1]; //16 bytes - address 20 to 35
    unsigned char partnumber[16+1]; //16 bytes - address 40 to 55
    unsigned char serial[16+1]; //16 bytes - address 68 to 83
    unsigned char date[8+1];//8 bytes - address 84 to 91
    int cwdm_wave;
    static char *connector[16] = //1 byte - address 2, pick from list
    {
        "Unknown",
        "SC",
        "Fibre Channel Style 1 copper connector",
        "Fibre Channel Style 2 copper connector",
        "BNC/TNC",
        "Fibre Channel coaxial headers",
        "FiberJack",
        "LC",
        "MT-RJ",
        "MU",
        "SG",
        "Optical pigtail"
    };

	printf("\nFibre Optic Dyanamic Test - LPA FOCat %0.1f\n\n",LPA_FOCat);

    //Copy eeprom SFP details into A50
    if(!read_eeprom(0x50));
    else exit(EXIT_FAILURE);
	
	//print identifier
	printf("Transciever Identifier = %s\n", identifier[A50[0]]); 
		
    //print the connector type
    printf("Connector Type = %s\n",connector[A50[2]]);

    //3 bytes, A50[60] is MSB, A50[61] is LSB &  A50[62] is fractional
    //print sfp wavelength
    cwdm_wave = ((int) A50[60]<<8) | ((int) A50[61]);
    printf("Wavelength = %d.%d\n",cwdm_wave,A50[62]);

    //print vendor id bytes 20 to 35
    memcpy(&vendor, &A50[20],16);
    vendor[16] = '\0';
    printf("Vendor = %s\n",vendor);

    //Print partnumber values address 40 to 55
    memcpy(&partnumber, &A50[40], 16);
    partnumber[16] = '\0';
    printf("Vendor Part Number = %s\n", partnumber);

    //Print serial values address 68 to 83
    memcpy(&serial, &A50[68], 16);
    serial[16] = '\0';
    printf("Serial = %s\n", serial);
    

    //Print date values address 84 to 91
    memcpy(&date, &A50[84], 8);
    date[8] = '\0';
    printf("Manufacture Date = %s\n\n", date);


    //If Digital Diagnostics is enabled and is Internally calibrated print
    //the DDM values.
    //if (A50[92] & 0x60) 
    //{
	//	  printf("____________________________________\n");
    //    ddm();
    //	  printf("____________________________________\n");
    //}
    return 0;
}

int dump(char *filename)
{
    int j;
    unsigned char index_start = 0;
    unsigned char index_end = 0;
    int i = 0;
    int counter = 0x0;
    FILE *Dumpfile;

    //Copy EEPROM SFP details into A50
    //EEPROM 
    if(!read_eeprom(0x50));
    else exit(EXIT_FAILURE);

    Dumpfile=fopen("EEPROM_contents","w");
    if (Dumpfile == NULL) 
    {
        fprintf (stderr, "Error opening file; errno = %s\n", strerror (errno));
        return 1;
    }
    printf ("Dumping to 'EEPROM_contents'...\n\n") ;
    fprintf(Dumpfile,"     0   1   2   3   4   5   6   7   8   9   a   b  "
            " c   d   e   f   0123456789abcdef");
    printf("     0   1   2   3   4   5   6   7   8   9   a   b  "
            " c   d   e   f   0123456789abcdef");
    while (counter < 0x100)
    {   //addresses 0 to 255
        if ((counter % 0x10) == 0)
        {
            index_end = counter;
            fprintf(Dumpfile,"  ");
            printf("  ");
            for(j = index_start; j <index_end; j++)
            {
                if(A50[index_start] == 0x0 ||
                        A50[index_start] == 0xff)
                {
					    fprintf(Dumpfile,".");
						printf(".");
                }
                else if (A50[index_start] < 32 ||
                         A50[index_start] >=127)
                {
                    fprintf(Dumpfile,"?");
                    printf("?");
                }
                else
                {
				    fprintf(Dumpfile,"%c",A50[index_start]);
					printf("%c",A50[index_start]);
                }
                index_start++;
            }
            index_start = index_end;
            fprintf(Dumpfile,"\n%02x:",i);
            printf("\n%02x:",i);
            i = i + 0x10;
        }
        fprintf(Dumpfile,"  %02x",A50[counter]);
        printf("  %02x",A50[counter]);
        counter = counter + 1;
    }
    fclose(Dumpfile);
    printf("\n");
    return 0;
}

int read_eeprom(unsigned char address)
{
    int xio,i,fd1;
    xio = wiringPiI2CSetup (address);
    if (xio < 0) 
    {
        fprintf (stderr, "xio: Can't initialise I2C: %s\n", strerror (errno));
        return 1;
    }
    //loop through addresses and extract data from EEPROM (A0h and A2h) 
    for(i=0; i <128; i++) 
		{
        fd1 = wiringPiI2CReadReg8 (xio,i);
        if  (address == 0x50) 
			{
				A50[i] = fd1;
			}
        else 
			{
				A51[i] = fd1;
			}
        if (fd1 <0) 
			{
				fprintf (stderr, "xio: Can't read i2c address 0x%x: %s\n", address, strerror (errno));
				return 1;
			}
		}
	//close xio to prevent segfault and "too many files open" errors
	//too many open file descriptors will be created by this loop
	close(xio);
    return 0;
}

int ddm(void)
{	
	//	96 Temp MSB, 	97 Temp LSB, 	98 Vcc MSB, 	99 Vcc LSB
	//	100 TX_BIA MSB,	101 TX_BIA LSB,
	//	102 TX MSB, 	103 TX LSB, 	104 RX MSB, 	105 RX LSB
    
    FILE *focat; // *datalog;
    float temperature, vcc, tx_bias, optical_tx, optical_rx, RAW_tx, RAW_rx;
	char ii[30], temp[10], vccc[10], txbi[10], optx[10], oprx[10], rwtx[30], rwrx[30], timex[30];
	int i, waittime, readtime;
	struct timeval stop, start, stop2, start2, startpause, stoppause;
	double secs, secs2, secspause = 0;
	
	//Open (or create) the csv file for datalogging
	focat=fopen("fcatdata.csv", "w");
		if(focat == NULL)
		{

        fprintf (stderr, "Error opening file; errno = %s\n", strerror (errno));
        return 1;
				
		}
		
	//Write header into csv file, close and reopen for appending only
	fprintf(focat,"i, Temp, Vcc, Tx_Bias, Tx, Rx, RAWTx, RAWRx, Time\n");
	fclose(focat);
	focat=fopen("fcatdata.csv", "a+");
	i=0;
	
	//start infinite loop
	gettimeofday(&start, NULL);
	for(;;)
	{
		
	//this is the timer for the inner while loop, reset every period
	gettimeofday(&start2, NULL);
	secs2=0;
	
	//this sets the time duration of each measuring period
	readtime=10;
	
	//this sets the wait duration between each measuring period
	waittime=40; 
	
		//start timed loop
		while (secs2<readtime)
		{	
		if(!read_eeprom(0x51));
		else exit(EXIT_FAILURE);
		i=i+1;

		//Taking MSB and LSB data and converting
		temperature =  (A51[96]+(float) A51[97]/256);
		vcc = 					(float)(A51[98]<<8  | A51[99])  * 0.0001;
		tx_bias = 				(float)(A51[100]<<8 | A51[101]) * 0.002;
		optical_tx = 10 * log10((float)(A51[102]<<8 | A51[103]) * 0.0001);
		optical_rx = 10 * log10((float)(A51[104]<<8 | A51[105]) * 0.0001);
		
		//Tx and Rx in mW - not too important
		RAW_tx = 	 		   ((float)(A51[102]<<8 | A51[103]) * 0.0001);
		RAW_rx = 	 	   	   ((float)(A51[104]<<8 | A51[105]) * 0.0001);
		
		//Timers for experiment duration and loop duration
		//Takes the time at this instant
		gettimeofday(&stop, NULL);
		gettimeofday(&stop2,NULL);
		
		//compares time at this instant to when the timer was first started
		//secs is for the begining of the experiment
		//secs2 is for the duration of the measuring time period
		secs =  (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
		secs = secs - secspause;
		secs2 = (double)(stop2.tv_usec - start2.tv_usec) / 1000000 + (double)(stop2.tv_sec - start2.tv_sec);
		
		//Display Diagnostics Monitoring Data in Terminal
		printf ("_______________________________________________\n");
		printf ("SFP Temperature = %4.4fC\n", temperature);
		printf ("Vcc, Internal supply = %4.4fV\n", vcc);
		printf ("TX bias current = %4.4fmA\n", tx_bias);
		printf ("Tx, Optical Power = %4.4f dBm", optical_tx);
		printf (", %6.6f mW\n", RAW_tx);
		printf ("Rx, Optical Power = %4.4f dBm", optical_rx);
		printf (", %6.6f mW\n", RAW_rx);
		printf ("Time elapsed %f", secs);
		printf (", Iteration %d \n", i);
		
		
		//Change the integers into strings for appending to file
		sprintf(ii,	  "%8.0d", i);
		sprintf(temp, "%4.4f", temperature);
		sprintf(vccc, "%4.4f", vcc);
		sprintf(txbi, "%4.4f", tx_bias);
		sprintf(optx, "%4.4f", optical_tx);
		sprintf(oprx, "%4.4f", optical_rx);
		sprintf(rwtx, "%6.6f", RAW_tx);
		sprintf(rwrx, "%6.6f", RAW_rx);
		sprintf(timex,"%8.6f", secs);
		
		//Appends DDM Data into a new row of a csv file
		fprintf(focat, "%s,%s,%s,%s,%s,%s,%s,%s,%s\n",ii,temp,vccc,txbi,optx,oprx,rwtx,rwrx,timex);
		
		
		//////////////put this in its own function?///////////////
		//////////////////////////////////////////////////////////
		//cable break detected - break out of the loop
		//done at the end to show cable break after saving to file
		//must consider the timers to ensure output data is continuous
		if (optical_rx == -40)
		{
			gettimeofday(&startpause,NULL);
			///////////needs fine tuning/////////
			//datalog=fopen("datalog.txt","a+");
			//fprintf(datalog,"%s,%?\n", "Start:",startpause);
			printf ("_______________________________________________\n");
			printf ("CABLE BREAK DETECTED\n");
			printf ("_______________________________________________\n");
			printf (" \n");
			control_off(); 		//stop the test
			control_break();	//provide user options						
			control_on();		//restart the test
			secs2=0;			
			gettimeofday(&stoppause,NULL);
			secspause =  secspause + (double)(stoppause.tv_usec - startpause.tv_usec) / 1000000 + (double)(stoppause.tv_sec - startpause.tv_sec);
			printf("Pause duration: %f\n",secspause);
				
			///////////fine tune this later///////////
			//fprintf(datalog, "%s,%?\n", "Stop:",stoppause);
			//fprintf(datalog, "%s,%?\n", "Duration:",secspause);
			//output some data to a file which only appends to a separate log file
			
			//put notes in main datalog for reference
			fprintf(focat, "%s\n", "PAUSED");
			fprintf(focat, "%s,%f\n", "Duration:",secspause);
			
		}
		//////////////////////////////////////////////////////////		
		///////////////end of cable break section/////////////////
		}
		
	printf(" \n");
	printf("_______________________________________________\n");	
	fprintf(focat, "%d iterations complete\n", i);
	printf("%d iterations complete\n", i);	
	printf("Waiting for %d seconds until next cycle... \n", waittime);
	printf(" \n");
	sleep(waittime);
	}
		
	fclose(focat);
return 0;
}

int control_on(void)
{
		//REF #define GPIO_pinon 0
		//this code points to pin 11 of the 40-pin header
		
	    if(wiringPiSetup() == -1) 
	    {
                printf("Failed to setup wiringPi!\n");
                return -1;
        }
		//send a signal to transistor by providing 3.3v power to
		//pin 11 of the header and allowing current to flow and
		//then output an off signal to the processor
        pinMode(GPIO_pinon, OUTPUT);
		digitalWrite(GPIO_pinon, 1);  // power on solenoid

		printf("'START' signal sent");
		
		delay(1000);			  	  // wait 1 sec
		digitalWrite(GPIO_pinon, 0);  // power off
        return 0;
}

int control_off(void)
{
		//REF #define GPIO_pinon 1
		//this code points to pin 12 of the 40-pin header
		
	    if(wiringPiSetup() == -1) 
	    {
                printf("Failed to setup wiringPi!\n");
                return -1;
        }
		//send a signal to transistor by providing 3.3v power to
		//pin 12 of the header and allowing current to flow and
		//then output an off signal to the processor
        pinMode(GPIO_pinoff, OUTPUT);
		digitalWrite(GPIO_pinoff, 1);  // power on 
		
		printf("'STOP' signal sent");
		
		delay(1000);			  	   // wait 1 sec
		digitalWrite(GPIO_pinoff, 0);  // power off
        return 0;
}

int control_break(void)
{
	char userin[200], userin2[200];
	int userans;
	printf("\nThe test has been stopped due to a broken cable\n");
	printf("Please deal with the broken cable appropriately\n");
	userans = 0;
	printf("\nWould you like to KILL the programme or CONTINUE? - ");
	//collects user input and then proceeds to kill or continue
	//since there is a safety aspect involved here, maybe it would be
	//a good idea to have a two-stage system in place using something 
	//like a push button?
	while (userans == 0)
	{
	fgets(userin,200,stdin);
		if (!strcmp(userin,"CONTINUE\n"))
		{
			printf("Confirm with 'YES' - ");
			fgets(userin2,200,stdin);
			if (!strcmp(userin2,"YES\n"))
			{
				printf("\nCONTINUING PROGRAMME...\n");
				userans = 1;
			}
			else 
			{
				printf("\nNot confirmed... Press Enter to return\n");
			}
		}
		
		else if (!strcmp(userin,"KILL\n"))
		{
			printf("Confirm with 'YES' - ");
			fgets(userin2,200,stdin);
			if (!strcmp(userin2,"YES\n"))
			{
				printf("\nKILLING PROGRAMME...\n");
				userans = 2;
			}
			else 
			{
				printf("\nNot confirmed... Press Enter to return\n");
			}
		}
		//SOMETHING IS WRONG HERE, THIS DOES NOT DISPLAY AFTER TYPING
		//ANYTHING OTHER THAN YES IN THE CONFIRM PROMPT
		else 
		{
			userans = 0;
			printf("Please enter either 'KILL' or 'CONTINUE' - "); 
		}
	}
	
	////////////////////////future work///////////////////////
	//call some push button function push_button() here
	//and wait for button press before applying cases
	//Connect to GPIO using resistors between 3.3v Power and
	//a listening pin
	delay(2000);
	switch (userans)
	{
		case 1:
			return 0;
			
		case 2:
			// do fclose for all;
			//fclose(focat);
			printf("Exiting\n");
			exit(0);
	}
	return 0;
}

