#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netdb.h>
#include<ifaddrs.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<linux/if_link.h>
#include<string.h>
#include"dc.h"

static struct ifc{
	/*ref*/const char*name;
	unsigned long long int rx_bytes,tx_bytes;
	/*own*/char*hostname;
	struct ifc*next;
}*ifcs;
//static struct ifc*ifcs;
static void ifcs_delete(){
	struct ifc*i=ifcs;
	while(i!=NULL){
		free(i->hostname);
		struct ifc*n=i->next;
		free(i);
		i=n;
	}
	ifcs=NULL;
}
static void ifcs_foreach(int f(struct ifc*)){
	struct ifc*i=ifcs;
	while(i!=NULL){
		const int ret=f(i);
		if(ret)break;
		i=i->next;
	}
}
static void ifcs_addfirst(/*takes*/struct ifc*i){
	if(ifcs){
		i->next=ifcs;
		ifcs=i;
		return;
	}
	ifcs=i;
	i->next=NULL;
}
static struct ifc*ifcs_getbyname(/*refs*/const char*name){
	struct ifc*i=ifcs;
	while(i!=NULL){
		if(!strncmp(i->name,name,NI_MAXHOST))
			return i;
		i=i->next;
	}
	i=(struct ifc*)calloc(1,sizeof(struct ifc));
	i->name=name;
	ifcs_addfirst(/*gives*/i);
	return i;
}
int net_main(struct dc*dc){
	struct ifaddrs*ifas,*ifa;
	if(getifaddrs(&ifas)==-1){perror("getifaddrs");return-1;}
	ifa=ifas;
	int ret=0;
	for(int n=0;ifa!=NULL;ifa=ifa->ifa_next,n++){
		if(ifa->ifa_addr==NULL)
			continue;
		const int family=ifa->ifa_addr->sa_family;
		const char*name=ifa->ifa_name;
		struct ifc*i=ifcs_getbyname(name);
		if(family==AF_PACKET&&ifa->ifa_data!=NULL){
			struct rtnl_link_stats*stats=ifa->ifa_data;
			i->rx_bytes=stats->rx_bytes;
			i->tx_bytes=stats->tx_bytes;
		}
//		if(family==AF_INET||family==AF_INET6){
		if(family==AF_INET){
			i->hostname=malloc(NI_MAXHOST);
			const int s=getnameinfo(ifa->ifa_addr,family==AF_INET?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6),i->hostname,NI_MAXHOST,NULL,0,NI_NUMERICHOST);
			if(s!=0){
//				printf("%s",gai_strerror(s));
//				ret=-2;
//				goto cleanup;
				continue;
			}
		}
	}
	int foo(struct ifc*i){
		char buf[1024];
		snprintf(buf,sizeof buf,"%s  %s  %llu/%llu KB",i->name,i->hostname?"up":"down",i->tx_bytes>>10,i->rx_bytes>>10);
		dccr(dc);
		dcdrwstr(dc,buf);
		return 0;
	}
	ifcs_foreach(foo);
//cleanup:
	freeifaddrs(ifas);
	ifcs_delete();
	return ret;
}
