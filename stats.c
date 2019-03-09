

#include <stdio.h>
#include <stats.h>

netstats stats;

static void protstat(char *prot, struct stats_proto *stat) 
{
	printf("%s%7d%6d%7d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n",
          prot, stat->xmit, stat->rexmit, stat->recv, stat->fw, stat->drop,
          stat->chkerr, stat->lenerr, stat->memerr, stat->rterr, stat->proterr,
          stat->opterr, stat->err);
}

static int netstat_proc(void *arg) 
{
	printf("     -------------- packets -------- ----------------- errors ----------------\n");
  	printf("       xmit rexmt   recv forwd  drop cksum   len   mem route proto   opt  misc\n");
  	printf("---- ------ ----- ------ ----- ----- ----- ----- ----- ----- ----- ----- -----\n");

  	protstat("link", &stats.link);
  	protstat("ip", &stats.ip);
  	protstat("icmp", &stats.icmp);
  	protstat("udp", &stats.udp);
  	protstat("tcp", &stats.tcp);
  	protstat("raw", &stats.raw);

  	return 0;
}

static int pbufs_proc(void *arg) 
{
	printf("Pool Available .. : %6d\n", stats.pbuf.avail);
  	printf("Pool Used ....... : %6d\n", stats.pbuf.used);
  	printf("Pool Max Used ... : %6d\n", stats.pbuf.max);
  	printf("Errors .......... : %6d\n", stats.pbuf.err);
  	printf("Reclaimed ....... : %6d\n", stats.pbuf.reclaimed);
  	printf("R/W Allocated ... : %6d\n", stats.pbuf.rwbufs);

  	return 0;
}

netstats* get_netstats() 
{
	return &stats;
}

void stats_init() 
{
	//memset(&stats, 0, sizeof(struct stats_all));
  
  	//register_proc_inode("pbufs", pbufs_proc, NULL);
  	//register_proc_inode("netstat", netstat_proc, NULL);
}



