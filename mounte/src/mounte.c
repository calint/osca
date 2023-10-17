#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/inotify.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<pwd.h>
#include<sys/mount.h>
#include<ctype.h>
#include <pwd.h>

#define EVENT_SIZE    (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024*(EVENT_SIZE+16))
char*trim(char*str){
	while(isspace(*str))
		str++;
	if(*str==0)
		return str;
	char*end=str+strlen(str)-1;
	while(end>str&&isspace(*end))
		end--;
	*(end+1)=0;
	return str;
}
int main(){
	puts("mounte");
	const uid_t uid=getuid();
	if(setuid(geteuid())){puts("cannot run as root");exit(1);}
	const int fd=inotify_init();
	if(fd<0)perror("inotify_init");
	inotify_add_watch(fd,"/dev",IN_CREATE|IN_DELETE);
	char buffer[EVENT_BUF_LEN];
	while(1){
		struct passwd*pw=getpwuid(uid);
		if(!pw){printf("%s:%d  nopwstruct\n",__FILE__,__LINE__);exit(1);};
		const char*uhomedir=pw->pw_dir;
		const int length=read(fd,buffer,EVENT_BUF_LEN);
		if(length<0){perror("read");exit(2);}
		for(int i=0;i<length;){
			struct inotify_event*event=(struct inotify_event*)&buffer[i];
			i+=EVENT_SIZE+event->len;
			if(!event->len)
				continue;
			if(event->mask&IN_ISDIR)
				continue;
			if(strncmp(event->name,"sd",2)||!isdigit(event->name[strlen(event->name)-1]))
				continue;
			char mntdir[256];
			snprintf(mntdir,sizeof mntdir,"%s/mnt/%s",uhomedir,event->name);//? unsafe
			if(event->mask&IN_CREATE){
				const int res1=mkdir(mntdir,0700);
				if(res1){
					printf("%s:%d  cannot mkdir %s\n",__FILE__,__LINE__,mntdir);
					perror("");
					exit(3);
				}
				char devpth[256];
				snprintf(devpth,sizeof devpth,"/dev/%s",event->name);
				FILE*f=fopen("/proc/filesystems","r");
				while(1){
					char line[256];
					if(!fgets(line,sizeof line,f))
						break;
					if(!strncmp(line,"nodev ",6))
						continue;
					const char*ftype=trim(line);
					const int res3=mount(devpth,mntdir,ftype,MS_SYNCHRONOUS|MS_NOATIME,0);
					 if(res3)
						continue;
					printf("mounte %s %s\n",mntdir,ftype);
					break;
				}
				fclose(f);
//				const int res2=chown(mntdir,uid,-1);
//				if(res2){printf("%s:%d  cannot chown\n",__FILE__,__LINE__);perror(0);}
//				if(chown(mntdir,uid,-1)){printf("cannot chown '%s'\n",mntdir);exit(2);}
			}else if(event->mask&IN_DELETE){
				printf("umount %s\n",mntdir);
				const int res2=umount(mntdir);
				if(res2){printf("%s:%d  cannot umount\n",__FILE__,__LINE__);perror(0);}
//				puts("rmdir");
				const int res1=rmdir(mntdir);
				if(res1){printf("%s:%d  cannot rmdir\n",__FILE__,__LINE__);perror(0);}
			}
		}
	}
}
