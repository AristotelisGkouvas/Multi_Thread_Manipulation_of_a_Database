#include <string.h>
#include "../engine/db.h"
#include "../engine/variant.h"
#include "bench.h"

#define DATAS ("testdb")

void *_write_test(void *arg)  								// Now we can call the function via threads.
{

	int i;
	double cost;
	long long start,end;
	Variant sk, sv; 
	DB* db;

	long int count;
	long int ncount;
	int thread_count;
	int r;

	struct data *w = (struct data *) arg;					// we create a new struct so we can take the values from the other struct.
	
	count = w->count;
	thread_count = w->thread_count;
	r = w->r;
	db = w->db;

	ncount = count/thread_count;							// new count is created because we want the threads to know the whole number.

	char key[KSIZE + 1];
	char val[VSIZE + 1];
	char sbuf[1024];

	memset(key, 0, KSIZE + 1);
	memset(val, 0, VSIZE + 1);
	memset(sbuf, 0, 1024);

	//db = db_open(DATAS);

	start = get_ustime_sec();
	for (i = 0; i < ncount; i++) {
		if (r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d adding %s\n", i, key);
		snprintf(val, VSIZE, "val-%d", i);

		sk.length = KSIZE;
		sk.mem = key;
		sv.length = VSIZE;
		sv.mem = val;

		db_add(db, &sk, &sv);
		if ((i % 10000) == 0) {
			fprintf(stderr,"random write finished %d ops%30s\r", i, "");
			fflush(stderr);
		}
	}

	//db_close(db);
	end = get_ustime_sec();
	cost = end -start;

	pthread_mutex_lock(&write_info);  						//we lock to keep safe random_w.
	random_w++;
	if(random_w==1){										// for stats.
		random_w_ncount = ncount;							
		random_w_cost = cost;
	}

	pthread_mutex_unlock(&write_info);						// we unlock.
	return 0;
}

void *_read_test(void *arg){								// Now we can call the function via threads.

	int i;
	int ret;
	int found = 0;
	double cost;
	long long start,end;
	Variant sk;
	Variant sv;
	DB* db;

	long int count;
	long int ncount;
	int thread_count;
	int r;

	struct data *rd = (struct data *) arg;					// we create a new struct so we can take the values from the other struct.
	
	count = rd->count;
	thread_count = rd->thread_count;
	r = rd->r;
	db = rd->db;

	ncount = count/thread_count;							// new count is created because we want the threads to know the whole number.

	char key[KSIZE + 1];

	//db = db_open(DATAS);

	start = get_ustime_sec();

	for (i = 0; i < ncount; i++) {
		memset(key, 0, KSIZE + 1);
		/* if you want to test random write, use the following */
		if (r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d searching %s\n", i, key);

		sk.length = KSIZE;
		sk.mem = key;

		ret = db_get(db, &sk, &sv);
		if (ret) {
			//db_free_data(sv.mem);
			found++;
		} else {
			INFO("not found key#%s", sk.mem);
    	}

		if ((i % 10000) == 0) {
			fprintf(stderr,"random read finished %d ops%30s\r", i, "");
			fflush(stderr);
		}
	}
	//db_close(db);

	end = get_ustime_sec();
	cost = end - start;

	pthread_mutex_lock(&read_info);							// we lock to keep safe random_rd.
	info = info + found;									// info summary.
	random_rd++;											
	if(random_rd==1){										// for stats
		random_rd_ncount = ncount;							
		random_rd_found = found;							
		random_rd_cost = cost;								
	}

	pthread_mutex_unlock(&read_info);						// we unlock.
	return 0;
}
