#include "myth_log.h"

int myth_textlog_entry_compare(const void *pa,const void *pb)
{
	myth_textlog_entry_t a,b;
	a=(myth_textlog_entry_t)pa;b=(myth_textlog_entry_t)pb;
	if (a->te > b->te)return 1;
	if (a->te < b->te)return -1;
	if (a->ts > b->ts)return 1;
	if (a->ts < b->ts)return -1;
	return 0;
}
