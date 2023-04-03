#include "bench.h"
#include <string.h>
#include <pthread.h>
#include "../engine/db.h"

#define DATAS ("testdb")

void _random_key(char *key,int length) {
	int i;
	char salt[36]= "abcdefghijklmnopqrstuvwxyz0123456789";

	for (i = 0; i < length; i++)
		key[i] = salt[rand() % 36];
}

void print_info(int info, long long total_time, long int write_count, long int read_count, int mode){

	printf(LINE1);
	printf("All threads are terminated after: %.6f seconds \n", (double)total_time);

	if (mode == 1){
		printf(LINE1);
		printf("| Writes (done:%ld): %.6f sec/op; %.1f reads/sec; cost:%.3f sec;\n",
			write_count,(double)((double)total_time / write_count),
			(double)(write_count / (double)total_time),
			(double)total_time);
		printf(LINE1);
		printf("|Random-Write (done:%ld): %.6f sec/op; %.1f writes/sec; cost:%.3fsec;\n"
			,random_w_ncount, (double)(random_w_cost / random_w_ncount)
			,(double)(random_w_ncount / random_w_cost)
			,random_w_cost);
		printf(LINE1); 
	}
	
	if (mode == 2){
		printf(LINE1);
		printf("| Reads (done:%ld, found:%d): %.6f sec/op; %.1f reads/sec cost:%.3f sec\n",
				read_count, info,
				(double)((double)total_time / read_count),
				(double)(read_count / (double)total_time),
				(double)total_time);
		printf(LINE1);
		printf("|Random-Read (done:%ld, found:%d): %.6f sec/op; %.1f reads /sec;  cost:%.3fsec\n",
			random_rd_ncount, random_rd_found,
			(double)(random_rd_cost / random_rd_ncount),
			(double)(random_rd_ncount / random_rd_cost),
			random_rd_cost);
		printf(LINE1);
	}
	
	if (mode == 3){
		printf(LINE1);
		printf("| Writes (done:%ld): %.6f sec/op; %.1f reads/sec; cost:%.3f sec;\n",
			write_count,(double)((double)total_time / write_count),
			(double)(write_count / (double)total_time),
			(double)total_time);
		printf(LINE1);
		printf(LINE1);
		printf("| Reads (done:%ld, found:%d): %.6f sec/op; %.1f reads/sec cost:%.3fsec\n",
				read_count, info,
				(double)((double)total_time / read_count),
				(double)(read_count / (double)total_time),
				(double)total_time);
		printf(LINE1);
		printf("|Random-Write (done:%ld): %.6f sec/op; %.1f writes/sec; cost:%.3fsec;\n"
			,random_w_ncount, (double)(random_w_cost / random_w_ncount)
			,(double)(random_w_ncount / random_w_cost)
			,random_w_cost);
		printf(LINE1); 
		printf("|Random-Read (done:%ld, found:%d): %.6f sec/op; %.1f reads /sec;  cost:%.3fsec\n",
			random_rd_ncount, random_rd_found,
			(double)(random_rd_cost / random_rd_ncount),
			(double)(random_rd_ncount / random_rd_cost),
			random_rd_cost);
		printf(LINE1);
	}
	
}

void _print_header(int count){

	double index_size = (double)((double)(KSIZE + 8 + 1) * count) / 1048576.0;
	double data_size = (double)((double)(VSIZE + 4) * count) / 1048576.0;

	printf("Keys:\t\t%d bytes each\n", 
			KSIZE);
	printf("Values: \t%d bytes each\n", 
			VSIZE);
	printf("Entries:\t%d\n", 
			count);
	printf("IndexSize:\t%.1f MB (estimated)\n",
			index_size);
	printf("DataSize:\t%.1f MB (estimated)\n",
			data_size);

	printf(LINE1);
}

void _print_environment(){

	time_t now = time(NULL);

	printf("Date:\t\t%s", 
			(char*)ctime(&now));

	int num_cpus = 0;
	char cpu_type[256] = {0};
	char cache_size[256] = {0};

	FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo) {
		char line[1024] = {0};
		while (fgets(line, sizeof(line), cpuinfo) != NULL) {
			const char* sep = strchr(line, ':');
			if (sep == NULL || strlen(sep) < 10)
				continue;

			char key[1024] = {0};
			char val[1024] = {0};
			strncpy(key, line, sep-1-line);
			strncpy(val, sep+1, strlen(sep)-1);
			if (strcmp("model name", key) == 0) {
				num_cpus++;
				strcpy(cpu_type, val);
			}
			else if (strcmp("cache size", key) == 0)
				strncpy(cache_size, val + 1, strlen(val) - 1);	
		}

		fclose(cpuinfo);
		printf("CPU:\t\t%d * %s", 
				num_cpus, 
				cpu_type);

		printf("CPUCache:\t%s\n", 
				cache_size);
	}
}

int main(int argc,char** argv){

	int i;
	long int count;
	long long start_time;
	long long end_time;
	long long total_time;
	long int thread_count;
	int total_rate;
	double new_rate;
	long int write_count;
	long int read_count;
	long int write_threads;
	long int read_threads;
	random_w_ncount = 0;								// variables for keeping stats of a random write thread.
	random_w_cost = 0;											
	random_rd_ncount = 0;								// variables for keeping stats of a random read thread.
	random_rd_cost = 0;
	random_rd_found = 0;
	random_rd = 0;                                      // variables for keeping stats of a random read thread.
	random_w = 0;                                       // variables for keeping stats of a random write thread. 
	info = 0;											// variables for keeping stats of read thread.
	DB* db; 											// we set a database. 

	pthread_mutex_init(&read_info,NULL);
	pthread_mutex_init(&write_info,NULL);


	srand(time(NULL));
	
	// the variable argc was < 3 before but we changed that to < 4 so we can count the threads
	if (argc < 4 && ((strcmp(argv[1], "write") == 0) || (strcmp(argv[1], "read") == 0))) { 
		fprintf(stderr,"Usage: db-bench <read | write> <count> <threads> \n");
		exit(1);
	}

	// we use < 5 to count the threads and the rate 
	if (argc < 5 && (strcmp(argv[1], "readwrite") == 0)) { 							
		fprintf(stderr,"Usage: db-bench <readwrite> <count> <threads> <rate of write[percentage]> \n");
		exit(1);
	}

	if (strcmp(argv[1], "write") == 0) {

		int r = 0;
	 	struct data w;									// the struct on the header file.
		count = atoi(argv[2]);							// number of the elements from command.
		thread_count = atoi(argv[3]);					// number of threads from command.
		pthread_t id[thread_count]; 					// sets an id for every thread.
		_print_header(count);
		_print_environment();

		if (argc == 5)
			r = 1;

		db = db_open(DATAS); 							// we open the database.
		w.count = count;
		w.thread_count = thread_count;
		w.db = db;
		w.r = r;

		start_time = get_ustime_sec();
		// write threads creation.
		for(i=0; i<thread_count; i++)
		{
			if(pthread_create(&id[i], NULL, _write_test, (void *) &w) !=0){
				printf("Create Pthread Error!\n");
				exit(1);
			}			
		}
		// wait for the threads to be terminated.
		for(i = 0; i<thread_count; i++)
		{
			if(pthread_join(id[i], NULL) !=0){
				printf("Pthread join Error!\n");
				exit(1);
			}
		}

		end_time = get_ustime_sec();
		total_time = end_time - start_time;
		print_info(0, total_time, count, 0, 1);

		//_write_test(count, r);
		db_close(db);

	} else if (strcmp(argv[1], "read") == 0) {

		int r = 0;
	 	struct data rd;
		count = atoi(argv[2]);							// number of the elements from command.
		thread_count = atoi(argv[3]);					// number of threads from command.
		pthread_t id[thread_count]; 					// sets an id for every thread.
		_print_header(count);
		_print_environment();

		if (argc == 5)
			r = 1;

		db = db_open(DATAS); 							// we open the database.
		rd.count = count;
		rd.thread_count = thread_count;
		rd.db = db;
		rd.r = r;

		start_time = get_ustime_sec();					// start time before reading.
		// read threads creation.
		for(i=0; i<thread_count; i++)
		{
			if(pthread_create(&id[i], NULL, _read_test, (void *) &rd) !=0){
				printf("Create pthread Error!\n");
				exit(1);
			}
		}
		// wait for the threads to be terminated.
		for(i = 0; i<thread_count; i++)
		{
			if(pthread_join(id[i], NULL) !=0){
				printf("pthread_join Error!\n");
				exit(1);
			}
		}

		end_time = get_ustime_sec();
		total_time = end_time - start_time;
		print_info(info, total_time, 0, count, 2);

		//_read_test(count, r);
		db_close(db);

	} else if (strcmp(argv[1], "readwrite")==0) {

		int r = 0;
		struct data w;									// the struct on the header file.
		struct data rd;									// the struct on the header file.

		count = atoi(argv[2]);							// number of the elements from command.
		thread_count = atoi(argv[3]);					// number of threads from command.
		total_rate = atoi(argv[4]);						// number for the rate from command.

		new_rate = ((double)total_rate/(double)100);
		write_count = count * new_rate;					// elements multiplied with the rate.
		read_count = count - write_count;				// elements minus rate.
		write_threads = thread_count * new_rate;   		// total threads for writing.
		read_threads = thread_count - write_threads;	// total threads for reading.
		pthread_t id[thread_count];						// sets an id for every thread.
		_print_header(count);
		_print_environment();

		if (argc == 6)
			r = 1;
		db = db_open(DATAS); 							// we open the database.

		w.count = write_count;
		w.thread_count = write_threads;
		w.db = db;
		w.r = r;

		rd.count = read_count;
		rd.thread_count = read_threads;
		rd.db = db;
		rd.r = r;

		start_time = get_ustime_sec();					// start time before writing.
		// write threads creation.
		for(i=0; i<write_threads; i++)
		{
			if(pthread_create(&id[i], NULL, _write_test, (void *) &w) !=0){
				printf("Create Pthread Error!\n");
				exit(1);
			}
		}
		// read threads creation.
		for(i = write_threads; i<thread_count; i++)
		{
			if(pthread_create(&id[i], NULL, _read_test, (void *) &rd) !=0){
				printf("Create Pthread Error!\n");
				exit(1);
			}
		}
		// wait for the threads to be terminated.
		for(i = 0; i<write_threads; i++)
		{
			if(pthread_join(id[i], NULL) !=0){
				printf("Pthread join Error!\n");
				exit(1);
			}
		}
       // wait for the threads to be terminated.
		for(i = write_threads; i<thread_count; i++)
		{
			if(pthread_join(id[i], NULL) !=0){
				printf("Pthread join Error!\n");
				exit(1);
			}
		}

		end_time = get_ustime_sec();
		total_time = end_time - start_time;
		
		print_info(info, total_time, write_count, read_count, 3);

		db_close(db);

	} else {
		fprintf(stderr,"Usage: db-bench <write | read> <count> <random>\n or Usage: db-bench <readwrite> <count> <threads> <rate of write[percentage]> \n");
		exit(1);
	}
	return 1;
}
