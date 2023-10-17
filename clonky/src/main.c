#define APP "clonky system overview"
#define _XOPEN_SOURCE 500
//#include"tmr.h"
#include"dc.h"
#include"graph.h"
#include"graphd.h"
#include"main-cfg.h"
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<ctype.h>
#include<dirent.h>
#include<time.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<signal.h>
#include<stdio.h>
static struct dc*dc;
static struct graph*graphcpu;
static struct graph*graphmem;
static struct graphd*graphwifi;
static char sys_cls_pwr_bat[64];
static char sys_cls_net_wlan[64];
static void sysvaluestr(const char*path,char*value,const int size){
	FILE*file=fopen(path,"r");
	if(!file){
		*value=0;
		return;
	}
	char fmt[32];
	snprintf(fmt,sizeof fmt,"%%%ds\\n",size);
	fscanf(file,fmt,value);
	fclose(file);
	char*p=value;
	while(*p){
		*p=tolower(*p);
		p++;
	}
}
static int sysvalueint(const char*path){
	FILE*file=fopen(path,"r");
	if(!file)return 0;
	int num;
	fscanf(file,"%d",&num);
	fclose(file);
	return num;
}
static long long sysvaluelng(const char*path){
	FILE*file=fopen(path,"r");
	if(!file)return 0;
	long long num;
	fscanf(file,"%lld\n",&num);
	fclose(file);
	return num;
}
static int sysvalueexists(const char*path){
	const int result=access(path,F_OK);
	return result!=-1?1:0;
}
static void strcompactspaces(char*s){
//	char*s=srcdest;
	char*d=s;
	if(!*s){*d=0;return;}
	while(*s==' ')s++;//? overrun
	do{
		*d++=*s;
		if(!*s)return;
		s++;
		const char isspc=*s==' ';
		while(*s==' ')s++;//? overrun
		if(isspc&&*s)*d++=' ';
	}while(1);
	*d=0;
}
//static void qdir(const char*path,void f(const char*)){
//	DIR*dir=opendir(path);
//	if(dir==NULL)
//		return;
//	struct dirent *dirent;
//	while((dirent=readdir(dir))!=NULL){
//		f(dirent->d_name);
//	}
//	closedir(dir);
//}
//static void netdir(const char*filename){
//	if(!filename)
//		return;
//	if(*filename=='.')
//		return;
//	char path[256];
//
//	*path=0;
//	strcat(path,"/sys/class/net/"); //? unsafe
//	strcat(path,filename);
//	strcat(path,"/operstate");
//	char operstate[64];
//	sysvaluestr(path,operstate,64);
//
//	*path=0;
//	strcat(path,"/sys/class/net/");
//	strcat(path,filename);
//	strcat(path,"/statistics/rx_bytes");
//	long long rx=sysvaluelng(path);
//
//	*path=0;
//	strcat(path,"/sys/class/net/");
//	strcat(path,filename);
//	strcat(path,"/statistics/tx_bytes");
//	long long tx=sysvaluelng(path);
//
//	if(!strcmp(operstate,"up")){
//		snprintf(bbuf,bbuf_len,"%s %s %lld/%lld KB",filename,operstate,tx>>10,rx>>10);
//	}else if(!strcmp(operstate,"dormant")){
//		snprintf(bbuf,bbuf_len,"%s %s",filename,operstate);
//	}else if(!strcmp(operstate,"down")){
//		snprintf(bbuf,bbuf_len,"%s %s",filename,operstate);
//	}else if(!strcmp(operstate,"unknown")){
//		snprintf(bbuf,bbuf_len,"%s %s %lld/%lld KB",filename,operstate,tx>>10,rx>>10);
//	}
//	dccr(dc);
//	dcdrwstr(dc,bbuf);
//}
//static void _rendlid(){
//	FILE*file=fopen("/proc/acpi/button/lid/LID/state","r");
//	if(file){
//		fgets(line,sizeof(line),file);
//		strcompactspaces(line);
//		snprintf(bbuf,bbuf_len,"lid %s",line);
//		dccr(dc);
//		dcdrwstr(dc,bbuf);
//		fclose(file);
//	}
//}
//static void _rendtherm2(){
//	file=fopen("/proc/acpi/thermal_zone/THM/temperature","r");
//	if(file){
//		fgets(line,sizeof(line),file);
//		strcompactspaces(line);
//		sprintf(bbuf,"%s",line);
//		dccr(dc);
//		dcdrwstr(dc,bbuf,strlen(bbuf)-1);
//		fclose(file);
//	}
//}
static void _rendhr(){
	dcdrwhr(dc);
}
const char sys_cls_pwr[]="/sys/class/power_supply/";
const char*energy_or_charge_prefix;
static void _rendbattery(){
	char buf[255]="";
	const int nchars=snprintf(buf,sizeof buf,"%s%s/%s_",sys_cls_pwr,sys_cls_pwr_bat,energy_or_charge_prefix);
	if(sizeof buf==nchars){printf("%s %d: probably truncated path: %s\n",__FILE__,__LINE__,buf);}
	const int maxlen=sizeof buf-nchars;
	char*p=buf+nchars;//? snprintf
	strncpy(p,"full",maxlen);
	const long long charge_full=sysvaluelng(buf);
	strncpy(p,"now",maxlen);
	const long long charge_now=sysvaluelng(buf);
	if(snprintf(buf,sizeof buf,"%s%s/status",sys_cls_pwr,sys_cls_pwr_bat)==sizeof buf){printf("%s %d: probably truncated path: %s\n",__FILE__,__LINE__,buf);}
	sysvaluestr(buf,buf,sizeof buf);
	dccr(dc);
	char bbuf[1024];
	snprintf(bbuf,sizeof bbuf,"battery %s  %lld/%lld mAh",buf,charge_now/1000,charge_full/1000);
	dcdrwstr(dc,bbuf);
	if(charge_full)
		dcdrwhr1(dc,width*charge_now/charge_full);
}
static void _rendcpuload(){
	static int cpu_total_last=0;
	static int cpu_usage_last=0;

	FILE*file=fopen("/proc/stat","r");
	if(!file)return;
	// user: normal processes executing in user mode
	// nice: niced processes executing in user mode
	// system: processes executing in kernel mode
	// idle: twiddling thumbs
	// iowait: waiting for I/O to complete
	// irq: servicing interrupts
	// softirq: servicing softirqs
	int user,nice,system,idle,iowait,irq,softirq;
	char bbuf[1024];
	fscanf(file,"%1024s %d %d %d %d %d %d %d\n",bbuf,&user,&nice,&system,&idle,&iowait,&irq,&softirq);
	fclose(file);
	const int total=(user+nice+system+idle+iowait+irq+softirq);
	const int usage=total-idle;
	const long long dtotal=total-cpu_total_last;
	cpu_total_last=total;
	const int dusage=usage-cpu_usage_last;
	cpu_usage_last=usage;
	const int usagepercent=dusage*100/dtotal;
	graphaddvalue(graphcpu,usagepercent);
	dcyinc(dc,dyhr);
	dcyinc(dc,default_graph_height);
	graphdraw2(graphcpu,dc,default_graph_height,100);
}
static void _rendhelloclonky(){
	static long long unsigned counter;
	counter++;
	char bbuf[1024];
	snprintf(bbuf,sizeof bbuf,"%llu hello%sclonky",counter,counter!=1?"s ":" ");
	dccr(dc);
	dcdrwstr(dc,bbuf);
}
static void _rendmeminfo(){
	FILE*file=fopen("/proc/meminfo","r");
	if(!file)return;
	char name[64],unit[32];
	long long memtotal,memavail;
	char bbuf[1024];
	fgets(bbuf,sizeof bbuf,file);//	MemTotal:        1937372 kB
	sscanf(bbuf,"%64s %lld %32s",name,&memtotal,unit);
	fgets(bbuf,sizeof bbuf,file);//	MemFree:           99120 kB
	fgets(bbuf,sizeof bbuf,file);//	MemAvailable:     887512 kB
	fclose(file);
	sscanf(bbuf,"%64s %lld %32s",name,&memavail,unit);
	int proc=(memtotal-memavail)*100/memtotal;
	graphaddvalue(graphmem,proc);
	dcyinc(dc,dyhr);
	dcyinc(dc,default_graph_height);
	graphdraw(graphmem,dc,2);
	if(memavail>>10!=0){
		memavail>>=10;
		memtotal>>=10;
		strcpy(unit,"MB");
	}
	snprintf(bbuf,sizeof bbuf,"freemem %llu of %llu %s",memavail,memtotal,unit);
	dccr(dc);
	dcdrwstr(dc,bbuf);
}
//static void _rendnet(){
//	qdir("/sys/class/net",netdir);
//}
static void _rendwifitraffic(){
	dcyinc(dc,default_graph_height+dyhr);
	char bbuf[1024];
	snprintf(bbuf,sizeof bbuf,"/sys/class/net/%s/statistics/tx_bytes",sys_cls_net_wlan);
	long long wifi_tx=sysvaluelng(bbuf);
	snprintf(bbuf,sizeof bbuf,"/sys/class/net/%s/statistics/rx_bytes",sys_cls_net_wlan);
	long long wifi_rx=sysvaluelng(bbuf);
	graphdaddvalue(graphwifi,wifi_tx+wifi_rx);
	graphddraw(graphwifi,dc,default_graph_height,wifigraphmax);
}
static void pl(const char*str){
	dccr(dc);
	dcdrwstr(dc,str);
}
static void _rendcheetsheet(){
static char*keysheet[]={
	"ĸey",
	"+c               console",
	"+f                 files",
	"+e                editor",
	"+m                 media",
	"+v                 mixer",
	"+i              internet",
	"+x                sticky",
	"+q              binaries",
//	"+prtsc          snapshot",
	"+p              snapshot",
	"",
	"đesktop",
	"+up                   up",
	"+down               down",
	"+shift+up        move-up",
	"+shift+down    move-down",
	"",
	"window",
	"+esc               close",
	"+b                  bump",
	"+s                center",
	"+w                 wider",
	"+W               thinner",
	"+r                resize",
	"+3            fullscreen",
	"+4           full height",
	"+5            full width",
	"+6   i-am-bored-surprise",
	"...                  ...",
	NULL};

	char**strptr=keysheet;
	while(*strptr){
		dccr(dc);
		dcdrwstr(dc,*strptr);
		strptr++;
	}
}
static void _renddf(){
	FILE*f=popen("df -h 2>/dev/null","r");
	if(!f)return;
	char bbuf[1024];
	while(1){
		if(!fgets(bbuf,sizeof bbuf,f))
			break;
		strcompactspaces(bbuf);
//		if(!strncmp(buf,"none ",5))
//			continue;
		if(bbuf[0]!='/')
			continue;
		pl(bbuf);
	}
	pclose(f);
}
static void _rendiostat(){
	static long long unsigned int last_kb_read=0,last_kb_wrtn=0;

	FILE*f=popen("iostat -d","r");
	if(!f)return;
	//	Linux 3.11.0-14-generic (vaio) 	03/12/2014 	_x86_64_	(2 CPU)
	//
	//	Device:            tps    kB_read/s    kB_wrtn/s    kB_read    kB_wrtn
	//	sda               7.89        25.40        80.46     914108    2896281
	char bbuf[1024];
	fgets(bbuf,sizeof bbuf,f);
	fgets(bbuf,sizeof bbuf,f);
	fgets(bbuf,sizeof bbuf,f);
	float tps,kb_read_s,kb_wrtn_s;
	long long int kb_read,kb_wrtn;
	char dev[64];
	fscanf(f,"%64s %f %f %f %lld %lld",dev,&tps,&kb_read_s,&kb_wrtn_s,&kb_read,&kb_wrtn);
	pclose(f);
	const char*unit="kB";
	snprintf(bbuf,sizeof bbuf,"read %lld %s wrote %lld %s",kb_read-last_kb_read,unit,kb_wrtn-last_kb_wrtn,unit);
	pl(bbuf);
	last_kb_read=kb_read;
	last_kb_wrtn=kb_wrtn;
}
static void _renddmsg(){
	FILE*f=popen("journalctl --lines=15 --no-pager","r");
//	FILE*f=popen("dmesg -t|tail -n10","r");
//	FILE*f=popen("tail -n10 /var/log/syslog","r");
	if(!f)return;
	char bbuf[1024];
	while(1){
		if(!fgets(bbuf,sizeof bbuf,f))
			break;
		pl(bbuf);
	}
	pclose(f);
}
static void _rendacpi(){
//	FILE*f=popen("acpi -batc|grep -vi 'no state information available'","r");
	FILE*f=popen("acpi -V|grep -vi 'no state information available'","r");
	if(!f)return;
	while(1){
		char bbuf[1024];
		if(!fgets(bbuf,sizeof bbuf,f))
			break;
		for(char*p=bbuf;*p;++p)*p=tolower(*p);
		pl(bbuf);
	}
	pclose(f);
}

#include"strb.h"
inline static void _renddatetime(){
	const time_t t=time(NULL);
	const struct tm*lt=localtime(&t);//? free?
	strb sb;strbi(&sb);
	if(strbp(&sb,asctime(lt)))return;
	dccr(dc);
	dcdrwstr(dc,sb.chars);
}

static void _rendcputhrottles(){
	FILE*f=fopen("/sys/devices/system/cpu/present","r");
	if(!f)return;
	int min,max;
	fscanf(f,"%d-%d",&min,&max);
	fclose(f);

	strb sb;strbi(&sb);
	if(strbp(&sb,"throttle "))return;

	for(int n=min;n<=max;n++){
		char bbuf[512];
		snprintf(bbuf,sizeof bbuf,"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq",n);
		const long long max_freq=sysvaluelng(bbuf);
		snprintf(bbuf,sizeof bbuf,"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",n);
		const long long cur_freq=sysvaluelng(bbuf);
		strbp(&sb," ");
		const long long proc=(cur_freq*100)/max_freq;
		strbfmtlng(&sb,proc);
		strbp(&sb,"%");
//		printf("%d  %lld    %lld   %lld\n",n,proc,cur_freq,max_freq);
	}
	pl(sb.chars);
}

static void _rendswaps(){
	FILE*f=fopen("/proc/swaps","r");
	if(!f)return;
//	Filename				Type		Size	Used	Priority
//	/dev/mmcblk0p3                          partition	2096124	16568	-1
	char bbuf[1024];
	fgets(bbuf,sizeof bbuf,f);
	char dev[64],type[32];
	long long size=0,used=0;
	if(!fscanf(f,"%64s %32s %lld %lld",dev,type,&size,&used))return;
	fclose(f);

	strb sb;strbi(&sb);
	if(strbp(&sb,"swapped "))return;
	if(strbfmtbytes(&sb,used<<10))return;
	pl(sb.chars);
}

static void autoconfig_bat(){
	DIR*dp=opendir("/sys/class/power_supply");
	if(!dp){
		puts("[!] battery: cannot open find dir /sys/class/power_supply");
		return;
	}
	struct dirent*ep;
	*sys_cls_net_wlan=0;
	while((ep=readdir(dp))){
		if(ep->d_name[0]=='.')continue;
		char cb[512];
		if(snprintf(cb,sizeof(cb),"/sys/class/power_supply/%s/type",ep->d_name)==sizeof cb){printf("%s %d - buffer probably overrun\n",__FILE__,__LINE__);}
		sysvaluestr(cb,cb,sizeof(cb));
		if(strcmp(cb,"battery"))continue;
		strncpy(sys_cls_pwr_bat,ep->d_name,sizeof sys_cls_pwr_bat);

		//? quirk if it energy_full_design  charge_full_design
		if(snprintf(cb,sizeof(cb),"/sys/class/power_supply/%s/energy_now",sys_cls_pwr_bat)==sizeof cb){printf("%s %d - buffer probably overrun\n",__FILE__,__LINE__);}
		if(sysvalueexists(cb)){
			energy_or_charge_prefix="energy";
			break;
		}
		if(snprintf(cb,sizeof(cb),"/sys/class/power_supply/%s/charge_now",sys_cls_pwr_bat)==sizeof cb){printf("%s %d - buffer probably overrun\n",__FILE__,__LINE__);}
		if(sysvalueexists(cb)){
			energy_or_charge_prefix="charge";
			break;
		}
		printf("%s %d - energy or charge not resolved\n",__FILE__,__LINE__);
		break;
	}
	closedir(dp);
	if(!*sys_cls_pwr_bat){
		puts("[!] no battery found in /sys/class/power_supply");
	}
	printf("· graphs battery: ");
	puts(sys_cls_pwr_bat);
	return;
}
static int is_wlan_device(const char*sys_cls_net_wlan){
	strb sb;strbi(&sb);
	if(strbp(&sb,"/sys/class/net/"))return 0;
	if(strbp(&sb,sys_cls_net_wlan))return 0;
	if(strbp(&sb,"/wireless"))return 0;


	struct stat s;
	if(stat(sb.chars,&s))return 0;
	return 1;
}
static void autoconfig_wifi(){
	DIR*dp=opendir("/sys/class/net");
	if(!dp){
		puts("[!] wifi: cannot open find dir /sys/class/net");
		return;
	}
	struct dirent*ep;
	*sys_cls_net_wlan=0;
	while((ep=readdir(dp))){
		if(ep->d_name[0]=='.')continue;
		if(!is_wlan_device(ep->d_name))continue;
		strncpy(sys_cls_net_wlan,ep->d_name,sizeof sys_cls_net_wlan);
		break;
	}
	closedir(dp);
	if(!*sys_cls_net_wlan){
		puts("[!] no wireless device found in /sys/class/net");
	}
	printf("· graphs network device: ");
	puts(sys_cls_net_wlan);
	return;
}
static void autoconfig(){
	autoconfig_bat();
	autoconfig_wifi();
}
#include"net.h"
static void on_draw(){
	dcyset(dc,ytop);
	dcclrbw(dc);
	_renddatetime();
	_rendcpuload();
	_rendhelloclonky();
	_rendmeminfo();
	_rendswaps();
	_rendwifitraffic();
//	_rendnet();
	net_main(dc);
	_rendhr();
	_rendiostat();
	_renddf();
	_rendhr();
	_rendcputhrottles();
	_rendbattery();
//	_rendlid();
	_rendhr();
	_rendacpi(dc);
	_rendhr();
	_renddmsg();
//	_rendsyslog();
	_rendhr();
	_rendhr();
	_rendcheetsheet();
	_rendhr();
	_rendhr();
	dcflush(dc);
}
#define exit_clean 1
static void sigexit(int i){
	puts("exiting");
	dcdel(dc);
	if(graphcpu)graphdel(graphcpu);
	if(graphmem)graphdel(graphmem);
	if(graphwifi)graphddel(graphwifi);
	signal(SIGINT,SIG_DFL);
	kill(getpid(),SIGINT);
	exit(i);
}
int main(){
	signal(SIGINT,sigexit);
	puts(APP);

//	const char*fmt="%16s %4d\n";
//	printf(fmt,"int",sizeof(int));
//	printf(fmt,"long int",sizeof(int));
//	printf(fmt,"long long",sizeof(int));

	if(!(dc=dcnew()))exit(1);
	dcwset(dc,width);
	if(align==1)
		dcxlftset(dc,dcwscrget(dc)-width);
	graphcpu=graphnew(width);
	graphmem=graphnew(width);
	graphwifi=graphdnew(width);
	autoconfig();
	//tmr(1,&on_draw);
	while(1){
		sleep(1);
		on_draw();
	}
}
