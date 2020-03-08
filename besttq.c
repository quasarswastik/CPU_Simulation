#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CITS2002 Project 1 2019
   Names:             Swastik Raj Chauhan , Ziwei Li
   Student numbers:   22556239 , 22439925
 */


//  besttq (v1.1)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c


//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES             4
#define MAX_DEVICE_NAME         20
#define MAX_PROCESSES           50
#define MAX_EVENTS_PER_PROCESS  100

#define TIME_CONTEXT_SWITCH     5
#define TIME_ACQUIRE_BUS        5


//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum                = 0;
int total_process_completion_time       = 0;

int speed[MAX_DEVICES];
char dev_name[MAX_DEVICES][MAX_DEVICE_NAME];
int dcount = 0;

int process_start[MAX_PROCESSES][2];
int pcount = 0;

int pmap[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int edetails1[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int edetails2[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int ecount = 0;
int eprocesscount[MAX_PROCESSES];

int exit_time[MAX_PROCESSES];
int simulation_no = 0;


//  ----------------------------------------------------------------------

#define CHAR_COMMENT            '#'
#define MAXWORD                 20

//print out the tracefile to check whether the arrays we define is correct
void printing()
{
	for (int i = 0 ; i < dcount ; i++){
		printf("device\t%s\t%i bytes/sec\n",dev_name[i],speed[i]);
	}
	printf("reboot\n");

	for (int j = 0 ; j < pcount ; j++){
		printf("process %i %i {\n",process_start[j][0],process_start[j][1]);
		for (int k = 0 ; k < eprocesscount[j]; k++){
			printf(" i/o\t%i\t%s\t%i\n",edetails1[j][k],dev_name[pmap[j][k]],edetails2[j][k]);
		}
		printf(" exit\t%i\n}\n",exit_time[j]);
	}
}

//a mapping function to find the position of i/o device in the device array
int mapping(char word[])
{
	for(int i = 0 ; i < dcount ; ++i)
	{
		if(strcmp(dev_name[i], word) == 0)
		{
			return i;
		}
	}
	printf("Error 1: Cannot find the device!");
	exit(EXIT_FAILURE);
}

void parse_tracefile(char program[], char tracefile[])
{
//  ATTEMPT TO OPEN OUR TRACEFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp    = fopen(tracefile, "r");

    if(fp == NULL) {
        printf("%s: unable to open '%s'\n", program, tracefile);
        exit(EXIT_FAILURE);
    }

    char line[BUFSIZ];
    int  lc     = 0;

//  READ EACH LINE FROM THE TRACEFILE, UNTIL WE REACH THE END-OF-FILE
    while(fgets(line, sizeof line, fp) != NULL) {
        ++lc;

//  COMMENT LINES ARE SIMPLY SKIPPED
        if(line[0] == CHAR_COMMENT) {
            continue;
        }

//  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        char    word0[MAXWORD], word1[MAXWORD], word2[MAXWORD], word3[MAXWORD];
        int nwords = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);

//      printf("%i = %s", nwords, line);

//  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if(nwords <= 0) {
            continue;
        }

    //  LOOK FOR LINES DEFINING DEVICES, PROCESSES, AND PROCESS EVENTS
	if(nwords == 4 && strcmp(word0, "device") == 0) {
	    strcpy(dev_name[dcount], word1);//store the devices' name from the tracefile
	    speed[dcount] = atoi(word2);//store the devices' speed
	    ++dcount;//calculate how many devices in the tracefile
        }

        else if(nwords == 1 && strcmp(word0, "reboot") == 0) {
            ;
        }

	//store the processes' name and their start time
	else if(nwords == 4 && strcmp(word0, "process") == 0) {
	       process_start[pcount][0] = atoi(word1);
	       process_start[pcount][1] = atoi(word2);
	       ++pcount;//count the number of total process
        }

	//  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
	else if(nwords == 4 && strcmp(word0, "i/o") == 0) {
		pmap[pcount-1][ecount] = mapping(word2); //store the position of each process's i/o devices in dev_name array
		edetails1[pcount-1][ecount] = atoi(word1); //store the start time of each process's i/o events
		edetails2[pcount-1][ecount] = atoi(word3); //store the amount of data transfered of each process's i/o events
		++ecount;//count how many events this process has

        }

	    //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
        else if(nwords == 2 && strcmp(word0, "exit") == 0) {
		exit_time[pcount-1] = atoi(word1);//store the exit time of each process
		eprocesscount[pcount-1] = ecount;//store the number of total events of each process
		ecount = 0;//once we come to the exit, we have counted all the events of the current process
        }

	   //  JUST THE END OF THE CURRENT PROCESS'S EVENTS
        else if(nwords == 1 && strcmp(word0, "}") == 0) {
            ;
        }
        else {
            printf("%s: line %i of '%s' is unrecognized",
                        program, lc, tracefile);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
}

#undef  MAXWORD
#undef  CHAR_COMMENT

//  ----------------------------------------------------------------------
//To implement bubble sort
void swap(int *xp, int *yp)
{
	int temp = *xp;
	*xp = *yp;
	*yp = temp;
}

//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
	printf("running simulate_job_mix( time_quantum = %i usecs )\n",
                time_quantum);

	//define arrays for ready queue and block queue
	int rq[pcount][7];
	int bq[pcount*MAX_EVENTS_PER_PROCESS][8];

	int number_of_exited_processes = 0;
	int time = 0; //the global time
	int cpu = -1; //indicate that CPU now is empty, when it is not, it will be the name of process in cpu
	int bus = -1;  //indicate that the databus is now empty
	int current_start = 0; //how long has a certain process been running in the CPU
	int counter2 = 0; //position of process in process_start array
	int io_not_bq = 0;//this counter calculates the number of i/o operations which are completed or is in the data bus or transition state( a state between block and bus)
	int process_acquire_time = 0; // time the process is being shifted to the cpu
	int rtoc = -1; // /indicate there is no process in transition from ready queue to cpu, when there is, rtoc will be the process name

	int t0; int t1; int t2; int t3; int t4; int t5; int t6; //swapping variables

	//we set up the ready queue containing all processes in the tracefile
	for (int i = 0 ; i < pcount ; i++)
	{
		rq[i][0] = process_start[i][1]; //the start time of the process
		rq[i][1] = exit_time[i]; //exit time of process
		rq[i][2] = 0; //the internal clock of the process
		rq[i][3] = process_start[i][0]; //the process name
		rq[i][4] = 0; //indicator of the block state
		rq[i][5] = counter2; //position of process in tracefile
		rq[i][6] = 0; // number of i/o requests in the blocked queue
		++counter2; //increasing the counter
	}

	int allio = 0; //the total number of i/o events
	int io_complete = 0; //total number of i/o events completed

	int start_time = rq[0][0]; //the start time of the first process
	time = rq[0][0]; //set the global time as the start time of the first process

	int real_p = pcount; // the actual number of processes that are in the ready queue

	//if all processes have exited
	while(number_of_exited_processes < pcount)
	{
		if(time >= rq[0][0]) // when the global time exceeds the start time of the first process in the ready queue
		{
			// shifting the process from ready queue to cpu
			if(cpu == -1 && rq[0][4] != -1 && rtoc == -1)
			{
				rtoc = rq[0][3]; //setting the variable as process name
				process_acquire_time = 0;
			}
			// process gets into cpu
			if (cpu == -1 && rq[0][4] != -1 && process_acquire_time == TIME_CONTEXT_SWITCH && rtoc == rq[0][3])
			{
				cpu = rq[0][3];
				current_start = rq[0][2]; // setting process internal clock time when the current process got into cpu
				printf("Ready -> Running p%i %i\n",cpu, time);
				rtoc = -1;
			}
		}

		//populating the blocked queue
		for (int i = rq[0][6] ; i < eprocesscount[rq[0][5]] ; ++i) //loop goes till last event in a process
		{
			// checks whether the internal clock of the process is equal to i/o start time of that process,
			// checking whether CPU is not empty and that i/o should not already be in the queue
			if (rq[0][2] == edetails1[rq[0][5]][i] && cpu == rq[0][3] && rq[0][6] < eprocesscount[rq[0][5]])
			{
				bq[allio][0] = pmap[rq[0][5]][i]; // stores the device position from dev_name array
				bq[allio][1] = edetails1[rq[0][5]][i]; // start time of i/o, added for future scope of the program
				bq[allio][2] = edetails2[rq[0][5]][i]; // total transfer by i/o, added for future scope of the program
				bq[allio][3] = rq[0][3]; // process name
				bq[allio][4] = ((edetails2[rq[0][5]][i]*10000-1)/(speed[pmap[rq[0][5]][i]]/100))+1; // time required for data bus
				bq[allio][5] = 0; // internal clock for i/o
				bq[allio][6] = allio;//position of i/o in the queue
				bq[allio][7] = 0;//the time of an i/o acquiring the databus
				++rq[0][6]; // increasing the i/o request put into blocked queue for the process
				++allio; // increment in total number of i/o requests in the blocked queue
			}
		}

		// goes from first blocked i/o request
		for (int i = io_complete ; i < allio ; ++i)
		{
			// checking if cpu has some process
			if (cpu == bq[i][3] && bq[i][1] == rq[0][2])
			{
				rq[0][4] = -1; // the process is blocked
				printf("Running -> Blocked p%i %i\n",bq[i][3],time);
				cpu = -1;

				for (int m = io_not_bq ; m < allio ; ++m)
				{
					bq[m][7] = 0;
				}

				// find the fastest i/o device and bubble sort it to the first position
				for (int re = 0; re < (allio-io_complete)-1; re++){
					for (int pe = 0; pe < (allio-io_complete)-re-1; pe++){
						if ((speed[bq[pe+1][0]]) > (speed[bq[pe][0]])){
							for (int qe = 0 ; qe < 8 ; qe++){
								swap(&bq[pe+1][qe], &bq[pe][qe]);
							}
						}
					}
				}

				// checking if swapping of process in ready queue is required
				if(real_p > 1)
				{
					int counter = 0; //checks for processes in the ready state, i.e., process have reached start time to ensure we shift
									 //the process to the position before the process hasn't started rather than to the end of the ready queue
					for (int i = 0 ; i < real_p ; ++i)
					{
						if (time > rq[i][0])
						{
							++counter;
						}
					}

					//swapping start here
					t0 = rq[0][0]; t1 = rq[0][1]; t2 = rq[0][2]; t3 = rq[0][3]; t4 = rq[0][4]; t5 = rq[0][5]; t6 = rq[0][6];
					for (int i = 0 ; i < counter-1 ; ++i)
					{
						rq[i][0] = rq[i+1][0]; rq[i][1] = rq[i+1][1]; rq[i][2] = rq[i+1][2]; rq[i][3] = rq[i+1][3]; rq[i][4] = rq[i+1][4];
						rq[i][5] = rq[i+1][5]; rq[i][6] = rq[i+1][6];
					}
					rq[counter - 1][0] = t0; rq[counter - 1][1] = t1; rq[counter - 1][2] = t2; rq[counter - 1][3] = t3; rq[counter - 1][4] = t4;
					rq[counter - 1][5] = t5; rq[counter - 1][6] = t6;
				}
			}
		}

		//increase the global time by 1
		++time;

		//starts data transfer for blocked process from the i/o hasn't been completed in the block queue
		for (int io_ops = io_complete ; io_ops < allio ; ++io_ops)
		{
			//check if the bus is empty and the current i/o and previous one have the same start time
			if (bus == -1 && bq[io_ops-1][1] != bq[io_ops][1])
			{
				++io_not_bq;//if the i/o goes to transfer state
			}

			//check whether the bus is empty
			if (bus == -1)
			{
				bus = bq[io_ops][3];
			}

			//check if the bus is occupied by the process and internal clock hasn't reached its time required
			//and number of io is equal to completed io and it has reached the time required for acquiring the bus
			//and it does not have the same started time as previous one
			//to ensure we only increase the internal clock for the i/o being transfered
			if (bus == bq[io_ops][3] && bq[io_ops][5] < bq[io_ops][4] && bq[io_ops][6] == io_complete && bq[io_ops][7] >= TIME_ACQUIRE_BUS && bq[io_ops-1][1] != bq[io_ops][1])// && bq[io_ops][4] == -1)
			{
				++bq[io_ops][5];//increase the internal clock of i/o
			}
			if (bus == bq[io_ops][3] && bq[io_ops][5] < bq[io_ops][4] && bq[io_ops][6] == io_complete && bq[io_ops-1][1] != bq[io_ops][1] && bq[io_ops][7] <= 5)// && bq[io_ops][4] == -1)
			{
				++bq[io_ops][7];
			}

			//check whether the i/o has completed its transfer by comparing its internal clock and the time required
			if (bus == bq[io_ops][3] && bq[io_ops][5] == bq[io_ops][4] && bq[io_ops][6] == io_complete)// && rq[0][4] == -1)
			{
				bus = -1;
				++io_complete;
				for (int p_compare = 0 ; p_compare < real_p ; ++p_compare)
				{
					//checks if process number of freed i/o is same as process number of blocked i/o
					if (bq[io_ops][3] == rq[p_compare][3])
					{
						rq[p_compare][4] = 0; // not blocked anymore
						printf("p%i.release_databus at time %i\n",rq[p_compare][3],time);
						bq[io_ops][7] = 0;
					}
				}
			}

			//check if the bus is occupied by the process and internal clock hasn't reached its time required
			//and number of io is equal to completed io to ensure we only increase the internal clock for the i/o being transfered
			if (bus == bq[io_ops][3] && bq[io_ops][5] < bq[io_ops][4] && bq[io_ops][6] == io_complete && bq[io_ops][7] >= 5 && bq[io_ops-1][1] == bq[io_ops][1])// && bq[io_ops][4] == -1)
			{
				++bq[io_ops][5];
			}
			if (bus == bq[io_ops][3] && bq[io_ops][5] < bq[io_ops][4] && bq[io_ops][6] == io_complete && bq[io_ops-1][1] == bq[io_ops][1] && bq[io_ops][7] <= 5)// && bq[io_ops][4] == -1)
			{
				++bq[io_ops][7];
			}
		}


		//if process still needs to execute,i.e., the internal clock hasn't reached the exit time,
		//and is running in the cpu within its time quantum and not blocked
		if(rq[0][2] < rq[0][1] && cpu == rq[0][3])
		{
			++rq[0][2];
		}

		if (process_acquire_time < TIME_CONTEXT_SWITCH)
		{
			++process_acquire_time;
		}


		//process exit when its internal clock has reached its exit time and is not in the block queue
		if(rq[0][2] == rq[0][1] && rq[0][4] != -1)
		{
			printf("Running -> Exit p%i at time = %i\n",cpu,time);
			cpu = -1;

			//removing the exit process from the ready queue
		    for (int c = -1; c < pcount - 1; c++)
		    {
		    	for (int rem_index = 0 ; rem_index < 7 ; ++rem_index)
		    	{
		    		rq[c][rem_index] = rq[c+1][rem_index];
		    	}
		    }
		    --real_p; //real number of process in the ready queue decreases
		    ++number_of_exited_processes; //number of exited process increases
		}

		// shift the ready queue after a process running over for a time quantum
		// and the global time exceeds the start time of next process in the ready queue
		if(((rq[0][2] - current_start) == time_quantum &&
				time > rq[1][0] && rq[1][4] > -1 && real_p > 1) && cpu != -1)
		{
			int counter = 0;
			for (int i = 0 ; i < real_p ; ++i){
				if (time > rq[i][0]){
					++counter;
				}
			}

			// shift
			t0 = rq[0][0]; t1 = rq[0][1]; t2 = rq[0][2]; t3 = rq[0][3]; t4 = rq[0][4]; t5 = rq[0][5]; t6 = rq[0][6];
			for (int i = 0 ; i < counter-1 ; ++i)
			{
				rq[i][0] = rq[i+1][0]; rq[i][1] = rq[i+1][1]; rq[i][2] = rq[i+1][2]; rq[i][3] = rq[i+1][3]; rq[i][4] = rq[i+1][4];
				rq[i][5] = rq[i+1][5]; rq[i][6] = rq[i+1][6];
			}
			rq[counter - 1][0] = t0; rq[counter - 1][1] = t1; rq[counter - 1][2] = t2; rq[counter - 1][3] = t3; rq[counter - 1][4] = t4;
			rq[counter - 1][5] = t5; rq[counter - 1][6] = t6;

			printf("p%i.expire = %i\n",cpu,time);
			cpu = -1;
		}
		if (rq[0][3] == cpu && (rq[0][2] - current_start) == time_quantum)
		{
			printf("p%i.freshTQ = %i\n",cpu,time);
			current_start = rq[0][2];
		}
	}

	//if it is first simulation
	if (simulation_no == 0)
	{
		optimal_time_quantum = time_quantum;
		total_process_completion_time = time - process_start[0][1];
	}

	//if newer time quanta is better than previous one
	if (time - start_time <= total_process_completion_time)
	{
		optimal_time_quantum = time_quantum;
		total_process_completion_time = time - start_time;
	}

	//increasing simulation
	++simulation_no;
}

//  ----------------------------------------------------------------------

void usage(char program[])
{
    printf("Usage: %s tracefile TQ-first [TQ-final TQ-increment]\n", program);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    int TQ0 = 0, TQfinal = 0, TQinc = 0;

//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND THREE TIME VALUES
    if(argcount == 5) {
        TQ0     = atoi(argvalue[2]);
        TQfinal = atoi(argvalue[3]);
        TQinc   = atoi(argvalue[4]);

        if(TQ0 < 1 || TQfinal < TQ0 || TQinc < 1) {
            usage(argvalue[0]);
        }
    }
//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND ONE TIME VALUE
    else if(argcount == 3) {
        TQ0     = atoi(argvalue[2]);
        if(TQ0 < 1) {
            usage(argvalue[0]);
        }
        TQfinal = TQ0;
        TQinc   = 1;
    }
//  CALLED INCORRECTLY, REPORT THE ERROR AND TERMINATE
    else {
        usage(argvalue[0]);
    }

//  READ THE JOB-MIX FROM THE TRACEFILE, STORING INFORMATION IN DATA-STRUCTURES
    parse_tracefile(argvalue[0], argvalue[1]);

    printing();
//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, VARYING THE TIME-QUANTUM EACH TIME.
//  WE NEED TO FIND THE BEST (SHORTEST) TOTAL-PROCESS-COMPLETION-TIME
//  ACROSS EACH OF THE TIME-QUANTA BEING CONSIDERED

   for(int time_quantum=TQ0 ; time_quantum<=TQfinal ; time_quantum += TQinc) {
        simulate_job_mix(time_quantum);
   }

//  PRINT THE PROGRAM'S RESULT
    printf("best %i %i\n", optimal_time_quantum, total_process_completion_time);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
