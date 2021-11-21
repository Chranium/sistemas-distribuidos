/* External definitions for job-shop model. */
// gcc jobshop.c simlib.c -o jobshop -lm

// gcc jobshop.c simlib.c -o jobshop -lm -fopenmp

#include "simlib.h"              /* Required for use of simlib.c. */
#include <omp.h>

#define EVENT_ARRIVAL         1  /* Event type for arrival of a job to the
                                    system. */
#define EVENT_DEPARTURE       2  /* Event type for departure of a job from a
                                    particular station. */
#define EVENT_END_SIMULATION  3  /* Event type for end of the simulation. */
#define STREAM_INTERARRIVAL   1  /* Random-number stream for interarrivals. */
#define STREAM_JOB_TYPE       2  /* Random-number stream for job types. */
#define STREAM_SERVICE        3  /* Random-number stream for service times. */
#define MAX_NUM_STATIONS      100  /* Maximum number of stations. */
#define MAX_NUM_JOB_TYPES     100  /* Maximum number of job types. */

/* Declare non-simlib global variables. */

int   num_stations, num_job_types, i, j, num_machines[MAX_NUM_STATIONS + 1],
      num_tasks[MAX_NUM_JOB_TYPES +1],
      route[MAX_NUM_JOB_TYPES +1][MAX_NUM_STATIONS + 1],
      num_machines_busy[MAX_NUM_STATIONS + 1], *job_type, *task;
float mean_interarrival, length_simulation,
      prob_distrib_job_type[MAX_NUM_JOB_TYPES + 1], // original prob_distrib_job_type [26]
      mean_service[MAX_NUM_JOB_TYPES +1][ MAX_NUM_STATIONS + 1];
FILE  *infile, *outfile;

/* Declare non-simlib functions. */

void  arrive(int new_job, int id_station);
void  depart(int id_station);
void  report(void);


int main(int argc, char **argv)  /* Main function. */
{

   char *fileInput =  "jobshop.input.txt";
   char *fileOutput = "jobshop.output.txt";

   if (argc == 2) {
      fileOutput = argv[1];
   }

   else if (argc == 3) {
      fileOutput = argv[1];
      fileInput = argv[2];
   }

   infile  = fopen(fileInput,  "r");
   outfile = fopen(fileOutput, "w");

    /* Read input parameters. */

    fscanf(infile, "%d %d %f %f", &num_stations, &num_job_types,
           &mean_interarrival, &length_simulation);
    for (j = 1; j <= num_stations; ++j)
        fscanf(infile, "%d", &num_machines[j]);
    for (i = 1; i <= num_job_types; ++i)
        fscanf(infile, "%d", &num_tasks[i]);
    for (i = 1; i <= num_job_types; ++i) {
        for (j = 1; j <= num_tasks[i]; ++j)
            fscanf(infile, "%d", &route[i][j]);
        for (j = 1; j <= num_tasks[i]; ++j)
            fscanf(infile, "%f", &mean_service[i][j]);
    }
    for (i = 1; i <= num_job_types; ++i)
        fscanf(infile, "%f", &prob_distrib_job_type[i]);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Job-shop model\n\n");
    fprintf(outfile, "Number of work stations%21d\n\n", num_stations);
    fprintf(outfile, "Number of machines in each station     ");
    for (j = 1; j <= num_stations; ++j)
        fprintf(outfile, "%5d", num_machines[j]);
    fprintf(outfile, "\n\nNumber of job types%25d\n\n", num_job_types);
    fprintf(outfile, "Number of tasks for each job type      ");
    for (i = 1; i <= num_job_types; ++i)
        fprintf(outfile, "%5d", num_tasks[i]);
    fprintf(outfile, "\n\nDistribution function of job types  ");
    for (i = 1; i <= num_job_types; ++i)
        fprintf(outfile, "%8.3f", prob_distrib_job_type[i]);
    fprintf(outfile, "\n\nMean interarrival time of jobs%14.2f hours\n\n",
            mean_interarrival);
    fprintf(outfile, "Length of the simulation%20.1f eight-hour days\n\n\n",
            length_simulation);
    fprintf(outfile, "Job type     Work stations on route");
    for (i = 1; i <= num_job_types; ++i) {
        fprintf(outfile, "\n\n%4d        ", i);
        for (j = 1; j <= num_tasks[i]; ++j)
            fprintf(outfile, "%5d", route[i][j]);
    }
    fprintf(outfile, "\n\n\nJob type     ");
    fprintf(outfile, "Mean service time (in hours) for successive tasks");
    for (i = 1; i <= num_job_types; ++i) {
        fprintf(outfile, "\n\n%4d    ", i);
        for (j = 1; j <= num_tasks[i]; ++j)
            fprintf(outfile, "%9.2f", mean_service[i][j]);
    }

    job_type = (int *) calloc(num_stations + 1, sizeof(int));
    task = (int *) calloc(num_stations + 1, sizeof(int));

    /* Initialize all machines in all stations to the idle state. */

    for (j = 1; j <= num_stations; ++j) {
      task[j] = 0;
      job_type[j] = 0;
      num_machines_busy[j] = 0;
    }

    /* Initialize simlib */

    init_simlib();

    /* Set maxatr = max(maximum number of attributes per record, 4) */

    maxatr = 4;  /* NEVER SET maxatr TO BE SMALLER THAN 4. */

    /* TODO: agregue comentario bonito en ingles */
    int threads = 1;

   //  int id_station, id_transfer;
   //  for (id_station = 1, id_transfer = 1; id_station <= 1; ++id_station, ++id_transfer)
    #pragma omp parallel num_threads(threads)
    {

      int id_station = omp_get_thread_num() + 1;
      int id_transfer = omp_get_thread_num() + 1;
        /* Schedule the arrival of the first job. */

        event_schedule(id_transfer, id_station, expon(mean_interarrival, STREAM_INTERARRIVAL),
                    EVENT_ARRIVAL);
        /* Schedule the end of the simulation.  (This is needed for consistency of
        units.) */

        event_schedule(id_transfer, id_station, 8 * length_simulation, EVENT_END_SIMULATION);

            /* Run the simulation until it terminates after an end-simulation event
       (type EVENT_END_SIMULATION) occurs. */
   
        do {

            /* Determine the next event. */

            timing(id_transfer, id_station);

            /* Invoke the appropriate event function. */

            switch (next_event_type[id_station]) {
                case EVENT_ARRIVAL:
                    arrive(1, id_station);
                    break;
                case EVENT_DEPARTURE:
                    depart(id_station);
                    break;
                case EVENT_END_SIMULATION:
                    // report();
                    break;
            }

        /* If the event just executed was not the end-simulation event (type
        EVENT_END_SIMULATION), continue simulating.  Otherwise, end the
        simulation. */

        } while (next_event_type[id_station] != EVENT_END_SIMULATION);
        #pragma omp barrier
    }

    report();
    fclose(infile);
    fclose(outfile);
    return 0;
}


void arrive(int new_job, int id_station)  /* Function to serve as both an arrival event of a job
                             to the system, as well as the non-event of a job's
                             arriving to a subsequent station along its
                             route. */
{
    int station;

    /* If this is a new arrival to the system, generate the time of the next
       arrival and determine the job type and task number of the arriving
       job. */


    if (new_job == 1) {
        job_type[id_station] = random_integer(prob_distrib_job_type, STREAM_JOB_TYPE);
        task[id_station]     = 1;
        station = route[job_type[id_station]][task[id_station]];
        // TODO: problemas con la variable station al reemplazar por id_station
        event_schedule(id_station, id_station, sim_time[id_station] + expon(mean_interarrival, STREAM_INTERARRIVAL),
                       EVENT_ARRIVAL);
        
    }

    /* Determine the station from the route matrix. */

    station = route[job_type[id_station]][task[id_station]];

    /* Check to see whether all machines in this station are busy. */

    if (num_machines_busy[station] == num_machines[station]) {

        /* All machines in this station are busy, so place the arriving job at
           the end of the appropriate queue. Note that the following data are
           stored in the record for each job:
             1. Time of arrival to this station.
             2. Job type.
             3. Current task number. */

        transfer[id_station][1] = sim_time[id_station];
        transfer[id_station][2] = job_type[id_station];
        transfer[id_station][3] = task[id_station];
        list_file(id_station, LAST, station, sim_time[id_station]);
    }

    else {

        /* A machine in this station is idle, so start service on the arriving
           job (which has a delay of zero). */

        sampst(0.0, station);                              /* For station. */
        sampst(0.0, num_stations + job_type[id_station]);              /* For job type. */
        ++num_machines_busy[station];
        timest((float) num_machines_busy[station], station, id_station);

        /* Schedule a service completion.  Note defining attributes beyond the
           first two for the event record before invoking event_schedule. */

        transfer[id_station][3] = job_type[id_station];
        transfer[id_station][4] = task[id_station];
        event_schedule(id_station, id_station, sim_time[id_station]
                       + erlang(2, mean_service[job_type[id_station]][task[id_station]],
                                STREAM_SERVICE),
                       EVENT_DEPARTURE);
    }
}


void depart(int id_station)  /* Event function for departure of a job from a particular
                      station. */
{
    int station, job_type_queue, task_queue;

    /* Determine the station from which the job is departing. */

    job_type[id_station] = transfer[id_station][3];
    task[id_station]     = transfer[id_station][4];
    station  = route[job_type[id_station]][task[id_station]];

    /* Check to see whether the queue for this station is empty. */

    if (list_size[station] == 0) { // as station.lenght == 0

        /* The queue for this station is empty, so make a machine in this
           station idle. */

        --num_machines_busy[station];
        timest((float) num_machines_busy[station], station, id_station);
    }

    else {

        /* The queue is nonempty, so start service on first job in queue. */

        list_remove(id_station, FIRST, station);

        /* Tally this delay for this station. */

        sampst(sim_time[id_station] - transfer[id_station][1], station);

        /* Tally this same delay for this job type. */

        job_type_queue = transfer[id_station][2];
        task_queue     = transfer[id_station][3];
        sampst(sim_time[id_station] - transfer[id_station][1], num_stations + job_type_queue);

        /* Schedule end of service for this job at this station.  Note defining
           attributes beyond the first two for the event record before invoking
           event_schedule. */

        transfer[id_station][3] = job_type_queue;
        transfer[id_station][4] = task_queue;
        //   station  = route[job_type_queue][task_queue];
        // event_schedule(id_station, station, sim_time
        //                + erlang(2, mean_service[job_type_queue][task_queue],
        //                         STREAM_SERVICE),
        //                EVENT_DEPARTURE);

        event_schedule(id_station, id_station, sim_time[id_station]
                       + erlang(2, mean_service[job_type_queue][task_queue],
                                STREAM_SERVICE),
                       EVENT_DEPARTURE);
    }

    /* If the current departing job has one or more tasks yet to be done, send
       the job to the next station on its route. */

    if (task[id_station] < num_tasks[job_type[id_station]]) {
        ++task[id_station];
        arrive(2, id_station);
    }
}


void report(void)  /* Report generator function. */
{
    int   i;
    float overall_avg_job_tot_delay, avg_job_tot_delay, sum_probs;

    /* Compute the average total delay in queue for each job type and the
       overall average job total delay. */

    fprintf(outfile, "\n\n\n\nJob type     Average total delay in queue");
    overall_avg_job_tot_delay = 0.0;
    sum_probs                 = 0.0;
    for (i = 1; i <= num_job_types; ++i) {
        avg_job_tot_delay = sampst(0.0, -(num_stations + i)) * num_tasks[i];
        fprintf(outfile, "\n\n%4d%27.3f", i, avg_job_tot_delay);
        overall_avg_job_tot_delay += (prob_distrib_job_type[i] - sum_probs)
                                     * avg_job_tot_delay;
        sum_probs = prob_distrib_job_type[i];
    }
    fprintf(outfile, "\n\nOverall average job total delay =%10.3f\n",
            overall_avg_job_tot_delay);

    /* Compute the average number in queue, the average utilization, and the
       average delay in queue for each station. */

    fprintf(outfile,
           "\n\n\n Work      Average number      Average       Average delay");
    fprintf(outfile,
             "\nstation       in queue       utilization        in queue");
    for (j = 1; j <= num_stations; ++j)
        fprintf(outfile, "\n\n%4d%17.3f%17.3f%17.3f", j, filest(j),
                timest(0.0, -j, 1) / num_machines[j], sampst(0.0, -j));
}

