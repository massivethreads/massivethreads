#ifndef MYTH_LOG_FUNC_H_
#define MYTH_LOG_FUNC_H_

#ifdef MYTH_COLLECT_LOG
extern FILE *g_log_fp;

static inline void myth_log_add_ws(myth_running_env_t env,myth_log_type_t type,int ws_victim)
{
	myth_log_entry_t e;
	if (type!=MYTH_LOG_QUIT && !g_log_worker_stat)return;
	e=&(env->log_data[env->log_count]);
	//store state and tsc to log
	e->tsc=get_rdtsc();
	e->type=type;
	e->ws_victim=ws_victim;
	env->log_count++;
	//Reallocate buffer if buffer is full
	if (env->log_count==env->log_buf_size){
		//myth_log_flush(env);
		env->log_buf_size*=2;
		env->log_data=realloc(env->log_data,sizeof(myth_log_entry)*env->log_buf_size);
	}
}

static inline void myth_log_add(myth_running_env_t env,myth_log_type_t type)
{
	myth_log_add_ws(env,type,-1);
}


static inline void myth_log_init(void)
{
#ifdef MYTH_LOG_EMIT_TEXTLOG
	char fname[1000];
	sprintf(fname,"myth-log-%d.txt",getpid());
	g_log_fp=fopen(fname,"w");
#endif
}
static inline void myth_log_fini(void)
{
	int i,j;
	FILE *fp_stat_out;
	myth_textlog_entry_t tx_logs;
	int textlog_count=0;
	int textlog_size=1;
	uint64_t min_ts=0xFFFFFFFFFFFFFFFFULL;
	uint64_t *idle_sum,*user_sum,idle_sum_all,user_sum_all;
	uint64_t *ws_count,ws_count_all;
	fp_stat_out=stdout;
#ifdef MYTH_LOG_EMIT_TEXTLOG
	//Emit category
	fprintf(g_log_fp,"Category[ index=0 name=Work_Stealing topo=Arrow color=(255,255,255,255,true) width=3 ]\n");
	fprintf(g_log_fp,"Category[ index=181 name=User_Computation topo=State color=(255,0,0,127,true) width=1 ]\n");
	fprintf(g_log_fp,"Category[ index=182 name=Thread_Idle topo=State color=(0,0,255,127,true) width=1 ]\n");
#endif
	tx_logs=malloc(sizeof(myth_textlog_entry)*textlog_size);
	idle_sum=malloc(sizeof(uint64_t)*g_worker_thread_num);
	user_sum=malloc(sizeof(uint64_t)*g_worker_thread_num);
	ws_count=malloc(sizeof(uint64_t)*g_worker_thread_num);
	//Merge all the logs and sort by time
	for (i=0;i<g_worker_thread_num;i++){
		int log_num;
		myth_log_entry_t e;
		idle_sum[i]=0;user_sum[i]=0;ws_count[i]=0;
		log_num=g_envs[i].log_count;
		e=g_envs[i].log_data;
		for (j=0;j<log_num-1;j++){
			myth_textlog_entry_t txe;
			uint64_t ts,te;
			int cat;
			txe=&tx_logs[textlog_count];
			ts=e[j].tsc;
			te=e[j+1].tsc;
			if (min_ts>ts)min_ts=ts;
			switch (e[j].type)
			{
			case MYTH_LOG_USER:
				if (e[j].ws_victim>0){
					ws_count[i]++;
					txe->ts=ts;
					txe->te=ts;
					txe->cat=0;
					txe->id_a=e[j].ws_victim;
					txe->id_b=i;
					//fprintf(g_log_fp,"Primitive[ TimeBBox(%llu,%llu) Category=0 (%llu, %d) (%llu, %d) ]\n",
					//					(unsigned long long)ts,(unsigned long long)ts,(unsigned long long)ts,e[j].ws_victim,(unsigned long long)ts,i);
					textlog_count++;
					if (textlog_count==textlog_size){
						textlog_size*=2;
						tx_logs=realloc(tx_logs,sizeof(myth_textlog_entry)*textlog_size);
					}
					txe=&tx_logs[textlog_count];
				}
				user_sum[i]+=te-ts;
				cat=181;
				break;
			case MYTH_LOG_WS:
				idle_sum[i]+=te-ts;
				cat=182;
				break;
			default:
				assert(0);
				break;
			}
			txe->ts=ts;
			txe->te=te;
			txe->cat=cat;
			txe->id_a=i;
			txe->id_b=i;
			//fprintf(g_log_fp,"Primitive[ TimeBBox(%llu,%llu) Category=%d (%llu, %d) (%llu, %d) ]\n",
			//		(unsigned long long)ts,(unsigned long long)te,cat,(unsigned long long)ts,i,(unsigned long long)te,i);
			textlog_count++;
			if (textlog_count==textlog_size){
				textlog_size*=2;
				tx_logs=realloc(tx_logs,sizeof(myth_textlog_entry)*textlog_size);
			}
		}
		assert(e[j].type==MYTH_LOG_QUIT);
	}
	qsort(tx_logs,textlog_count,sizeof(myth_textlog_entry),myth_textlog_entry_compare);
	//Emit logs
	for (i=0;i<textlog_count;i++){
		tx_logs[i].te-=min_ts;
		tx_logs[i].ts-=min_ts;
	}
#ifdef MYTH_LOG_EMIT_TEXTLOG
	for (i=0;i<textlog_count;i++){
		fprintf(g_log_fp,"Primitive[ TimeBBox(%llu,%llu) Category=%d (%llu, %d) (%llu, %d) ]\n",
				(unsigned long long)tx_logs[i].ts,(unsigned long long)tx_logs[i].te,
				tx_logs[i].cat,(unsigned long long)tx_logs[i].ts,
				tx_logs[i].id_a,(unsigned long long)tx_logs[i].te,tx_logs[i].id_b);
	}
	free(tx_logs);
	fclose(g_log_fp);
#endif
	idle_sum_all=user_sum_all=ws_count_all=0;
	for (i=0;i<g_worker_thread_num;i++){
#ifdef MYTH_LOG_EMIT_STAT_WORKER
		fprintf(fp_stat_out,"Worker %d : %llu,%llu,%lf (%llu Work-Stealing)\n",i,(unsigned long long)user_sum[i],(unsigned long long)idle_sum[i],(double)user_sum[i]/(double)(idle_sum[i]+user_sum[i]),(unsigned long long)ws_count[i]);
#endif
		idle_sum_all+=idle_sum[i];user_sum_all+=user_sum[i];ws_count_all+=ws_count[i];
	}
#ifdef MYTH_LOG_EMIT_STAT_ALL
	fprintf(fp_stat_out,"Total work-stealing count : %llu ( %lf per core)\n",(unsigned long long)ws_count_all,(double)ws_count_all/(double)g_worker_thread_num);
	fprintf(fp_stat_out,"Total user time : %llu ( %lf per core)\n",(unsigned long long)user_sum_all,(double)user_sum_all/(double)g_worker_thread_num);
	fprintf(fp_stat_out,"Total idle time : %llu ( %lf per core)\n",(unsigned long long)idle_sum_all,(double)idle_sum_all/(double)g_worker_thread_num);
#endif
	free(idle_sum);
	free(user_sum);
	free(ws_count);
	for (i=0;i<g_worker_thread_num;i++){
		free(g_envs[i].log_data);
	}
}
static inline void myth_log_worker_init(myth_running_env_t env)
{
	env->log_buf_size=MYTH_LOG_INITIAL_BUFFER_SIZE;
	env->log_data=malloc(sizeof(myth_log_entry)*env->log_buf_size);
	env->log_count=0;
}
static inline void myth_log_worker_fini(myth_running_env_t env)
{
}
#else
static inline void myth_log_add_ws(myth_running_env_t env,myth_log_type_t type,int ws_victim)
{
}
static inline void myth_log_add(myth_running_env_t env,myth_log_type_t type)
{
}
static inline void myth_log_init(void)
{
}
static inline void myth_log_fini(void)
{
}
static inline void myth_log_worker_init(myth_running_env_t env)
{
}
static inline void myth_log_worker_fini(myth_running_env_t env)
{
}
#endif


#endif /* MYTH_LOG_FUNC_H_ */
