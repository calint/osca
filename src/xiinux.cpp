#define APP "xiinux web server"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<errno.h>
#include<signal.h>
#include<unistd.h>
#include<sys/sendfile.h>
#include<pthread.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/stat.h>
//#include<thread>
#include<netinet/tcp.h>
#include<typeinfo>
static int const K=1024;
static size_t const conbufnn=K;
static int const nclients=K;
static int const port=8088;
class stats{
public:
	size_t ms{0};
	size_t socks{0};
	size_t sessions{0};
	size_t requests{0};
	size_t input{0};
	size_t output{0};
	size_t accepts{0};
	size_t reads{0};
	size_t writes{0};
	size_t files{0};
	size_t widgets{0};
	size_t cache{0};
	size_t errors{0};
	size_t brkp{0};
	void printhdr(FILE*f){
		fprintf(f,"%12s%12s%12s%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s\n","ms","input","output","socks","reqs","sess","accepts","reads","writes","files","widgets","cache","errors","brkp");
		fflush(f);
	}
	void print(FILE*f){
		fprintf(f,"\r%12zu%12zu%12zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu",ms,input,output,socks,requests,sessions,accepts,reads,writes,files,widgets,cache,errors,brkp);
		fflush(f);
	}
};
static stats stats;
const char*exception_connection_reset_by_client="brk";
size_t io_send(int fd,const void*buf,size_t len,bool throw_if_send_not_complete=false){
	stats.writes++;
	const ssize_t n=send(fd,buf,len,MSG_NOSIGNAL);
	if(n<0){
		if(errno==EPIPE||errno==ECONNRESET){
			stats.brkp++;
//			throw"brk";
			throw exception_connection_reset_by_client;
		}
		stats.errors++;
		throw"iosend";
	}
	stats.output+=(size_t)n;
	if(throw_if_send_not_complete&&(size_t)n!=len){
		stats.errors++;
		throw"sendnotcomplete";
	}
	return(size_t)n;
}
class xwriter{
	int fd;
	const char*set_session_id{nullptr};
public:
	xwriter(const int fd=0):fd(fd){}
	inline void send_session_id_at_next_opportunity(const char*id){set_session_id=id;}
	xwriter&reply_http(int code,const char*content,size_t len){
		char bb[K];
		int n;
		if(set_session_id){
			// Connection: Keep-Alive\r\n  for apache bench
			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\nSet-Cookie: i=%s;Expires=Wed, 09 Jun 2021 10:18:14 GMT\r\n\r\n",code,len,set_session_id);
			set_session_id=nullptr;
		}else{
			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\n\r\n",code,len);
		}
		if(n<0)throw"send";
		pk(bb,(size_t)n).pk(content,len);
		return*this;
	}
	xwriter&reply_http(int code,const char*content){
		const size_t nn=strlen(content);
		return reply_http(code,content,nn);
	}
	xwriter&pk(const char*s,const size_t nn){
		io_send(fd,s,nn,true);
		return*this;
	}
	inline xwriter&pk(const char*s){
		const size_t snn=strlen(s);
		return pk(s,snn);
	}
};
class doc{
	size_t size;
	char*buf;
//	const char*lastmod;
public:
	doc(const char*data,const char*lastmod=nullptr){//:lastmod(lastmod){
//		printf("new doc %p\n",(void*)this);
		size=strlen(data);
		buf=(char*)malloc(size);
		memcpy(buf,data,size);
	}
	~doc(){
//		printf(" * delete doc %p\n",(void*)this);
		free(buf);
//		delete buf;
	}
	inline const char*getbuf()const{return buf;}
	inline size_t getsize()const{return size;}
	inline void to(xwriter&x)const{x.pk(buf,size);}
};
static doc*homepage;

class widget{
public:
	virtual ~widget(){
//		printf(" * delete widget %s  @  %p\n",typeid(*this).name(),(void*)this);
	};
	virtual void to(xwriter&x)=0;
	virtual void ax(xwriter&x,char*a[]=0){if(a)x.pk(a[0]);}
	virtual void on_content(xwriter&x,/*local*/const char*content,const size_t content_len){};
};
static char*strtrm(char*p,char*e){
	while(p!=e&&isspace(*p))
		p++;
	while(p!=e&&isspace(*e))
		*e--=0;
	return p;
}
static void strlwr(char*p){
	while(*p){
		*p=(char)tolower(*p);
		p++;
	}
}
template<class T>class lut{
private:
	unsigned int size;
	class el{
	public:
		char*key{nullptr};
		T data{nullptr};
		el*nxt{nullptr};
		el(char*key,T data):key(key),data(data){
//			printf(" * new lut element %s @ %p\n",key,(void*)this);		
		}
		~el(){
//			printf("delete lut element %s  %s @ %p\n",key,typeid(*this).name(),(void*)this);
//			printf("delete lut element %s  @ %p\n",key,(void*)this);
//			printf("delete lut element @ %p\n",(void*)this);
			if(nxt)
				delete nxt;
		}
		void delete_content_recurse(bool delete_key){
//			printf("delete lut data %s @ %p\n",typeid(*data).name(),(void*)this);
			if(data)
				delete data;
			if(delete_key)
				free(key);
			if(!nxt)
				return;
			nxt->delete_content_recurse(delete_key);
		}
	};
	el**array;
public:
	static unsigned int hash(const char*key,const unsigned int roll){
		unsigned int i=0;
		const char*p=key;
		while(*p)
			i+=(unsigned int)*p++;
		i%=roll;
		return i;
	}
	lut(const unsigned int size=8):size(size){
//		printf("new lut %p\n",(void*)this);
		array=(el**)calloc(size_t(size),sizeof(el*));
	}
	~lut(){
//		printf("delete lut %p\n",(void*)this);
		clear();
		free(array);
	}
	T operator[](const char*key){
		const unsigned int h=hash(key,size);
		el*e=array[h];
		if(!e)
			return nullptr;
		while(true){
			if(!strcmp(e->key,key)){
				return e->data;
			}
			if(e->nxt){
				e=e->nxt;
				continue;
			}
			return nullptr;
		}
	}
	void put(char*key,T data,bool allow_overwrite=true){
		const unsigned int h=hash(key,size);
		el*l=array[h];
		if(!l){
			array[h]=new el(key,data);
			return;
		}
		while(1){
			if(!strcmp(l->key,key)){
				if(!allow_overwrite)
					throw"lutoverwrite";
				l->data=data;
				return;
			}
			if(l->nxt){
				l=l->nxt;
				continue;
			}
			l->nxt=new el(key,data);
			return;
		}
	}
	void clear(){
//		printf("clear lut %p\n",(void*)this);
		for(unsigned int i=0;i<size;i++){
			el*e=array[i];
			if(!e)
				continue;
			delete e;
			array[i]=nullptr;
		}
	}
	void delete_content(const bool delete_keys){
//		printf("delete lut content %p\n",(void*)this);
		for(unsigned int i=0;i<size;i++){
			el*e=array[i];
			if(!e)
				continue;
			e->delete_content_recurse(delete_keys);
			delete e;
			array[i]=nullptr;
		}
	}
};
class session{
	char*_id;
	lut<char*>kvp;
	lut<widget*>widgets;
public:
	session(/*takes*/char*session_id):_id(session_id){
		stats.sessions++;
//		printf(" * new session %s @ %p\n",session_id,(void*)this);
	}
	~session(){
		stats.sessions--;
//		printf(" * delete session %s\n",_id);
		free(_id);
		kvp.delete_content(true);
		widgets.delete_content(true);
	}
	inline const char*id()const{return _id;}
	inline void*operator[](const char*key){return kvp[key];}
	inline void put(char*key,/*takes*/char*data){kvp.put(key,data);}
	inline widget*get_widget(const char*key){return widgets[key];}
	inline void put_widget(char*key,/*takes*/widget*o){widgets.put(key,o);}
};
class sessions{
public:
	~sessions(){
//		printf(" * delete sessions %p\n",(void*)this);
		all.delete_content(false);
	}
	lut<session*>all{K};
};
static sessions sessions;
static widget*widgetget(const char*qs);
static int epollfd;
static void urldecode(char*str){
	char a,b;
	const char*p=str;
	while(*p){
		           //? unsafe access
		if(*p=='%'&&(a=p[1])&&(b=p[2])&&isxdigit(a)&&isxdigit(b)){
			if(a>='a')a-='a'-'A';
			if(a>='A')a-=('A'-10);
			else a-='0';
			if(b>='a')b-='a'-'A';
			if(b>='A')b-=('A'-10);
			else b-='0';
			*str++=16*a+b;
			p+=3;
			continue;
		}
		*str++=*p++;
	}
	*str++='\0';
}
class sock{
	enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,read_content,upload,next_request};
	parser_state state{method};
	int file_fd{0};
	off_t file_pos{0};
	size_t file_len{0};
	int upload_fd{0};
	char*pth{nullptr};
	char*qs{nullptr};
	lut<const char*>hdrs;
	char*hdrk{nullptr};
	char*hdrv{nullptr};
	char*content{nullptr};
	size_t content_len{0};
	size_t content_pos{0};
	char buf[conbufnn];
	char*bufp{buf};
	size_t bufi{0};
	size_t bufnn{0};
	void reset_for_new_request(){
		file_fd=0;
		file_pos=0;
		file_len=0;
		upload_fd=0;
		pth=nullptr;
		qs=nullptr;
		hdrs.clear();
		hdrk=nullptr;
		hdrv=nullptr;
		content=nullptr;
		content_len=0;
		content_pos=0;
		//? clear buf
		bufp=buf;
		bufi=0;
		bufnn=0;
	}
	void io_request_read(){
		struct epoll_event ev;
		ev.data.ptr=this;
		ev.events=EPOLLIN;
		if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)){
			perror("ioreqread");
			throw"epollmodread";
		}
	}
	void io_request_write(){
		struct epoll_event ev;
		ev.data.ptr=this;
		ev.events=EPOLLOUT;
		if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)){
			perror("ioreqwrite");
			throw"epollmodwrite";
		}
	}
public:
	int fd{0};
	sock(const int f=0):fd(f){
		stats.socks++;
	}
	~sock(){
		stats.socks--;
		//printf(" * delete sock %p\n",(void*)this);
		delete[]content;
		if(!::close(fd)){
			return;
		}
		stats.errors++;
		printf("%s:%d ",__FILE__,__LINE__);perror("sockdel");
	}
	void close(){
		::close(fd);
	}
	void run(){while(true){
		//printf(" state %d\n",state);
		if(state==upload){
			char upload_buffer[4*K];
			const size_t upload_remaining=content_len-content_pos;
			stats.reads++;
			const ssize_t nn=recv(fd,upload_buffer,upload_remaining>sizeof upload_buffer?sizeof upload_buffer:upload_remaining,0);
			if(nn==0){//closed by client
				throw"brk";
			}
			if(nn<0){
				if(errno==EAGAIN||errno==EWOULDBLOCK){
//					printf("eagain || wouldblock\n");
					io_request_read();
					return;
				}else if(errno==ECONNRESET){
					throw"brk";
				}
				printf("\n\n%s:%d ",__FILE__,__LINE__);
				perror("upload");
				stats.errors++;
				throw"upload";
			}
			const ssize_t nw=write(upload_fd,upload_buffer,(size_t)nn);
			if(nw<0){
				printf("\n\n%s:%d ",__FILE__,__LINE__);
				perror("writing upload to file");
				stats.errors++;
				throw"writing upload to file";
			}
			if(nw!=nn){
				throw"writing upload to file 2";
			}
			stats.input+=(size_t)nn;
			content_pos+=(size_t)nn;
//			printf(" uploading %s   %zu of %zu\n",pth+1,content_pos,content_len);
			if(content_pos<content_len){
				continue;
			}
			if(::close(upload_fd)<0){
				perror("while closing file");
			}
//			printf("      done %s\n",pth+1);
			io_send(fd,"HTTP/1.1 204\r\n\r\n",16,true);
			const char*str=hdrs["connection"];
			if(!str||strcmp("Keep-Alive",str)){
				delete this;
				return;
			}
			state=method;
			io_request_read();
			return;
			//?? chained requests
		}else if(state==read_content){
			stats.reads++;
			const ssize_t nn=recv(fd,content+content_pos,content_len-content_pos,0);
			if(nn==0){//closed by client
				delete this;
				return;
			}
			if(nn<0){
				if(errno==EAGAIN||errno==EWOULDBLOCK){
//					printf("eagain || wouldblock\n");
					io_request_read();
					return;
				}else if(errno==ECONNRESET){
					throw"brk";
				}
				printf("\n\n%s:%d ",__FILE__,__LINE__);perror("readcontent");
				stats.errors++;
				throw"readingcontent";
			}
			content_pos+=(unsigned)nn;
			stats.input+=(unsigned)nn;
			if(content_pos==content_len){
				*(content+content_len)=0;
				process();
				break;
			}
			return;
		}else if(state==resume_send_file){
			stats.writes++;
			const ssize_t sf=sendfile(fd,file_fd,&file_pos,file_len);
			if(sf<0){//error
				if(errno==EAGAIN){
					io_request_write();
					return;
				}
				stats.errors++;
				printf("\n\n%s:%d ",__FILE__,__LINE__);perror("resumesendfile");
				delete this;
				return;
			}
			file_len-=size_t(sf);
			stats.output+=size_t(sf);
			if(file_len!=0){
				state=resume_send_file;
				io_request_write();
				return;
			}
			::close(file_fd);
			state=method;
		}
		if(bufi==conbufnn)
			throw"bufferoverrun";
		if(bufi==bufnn){
			bufi=bufnn=0;
			bufp=buf;
			stats.reads++;
			const ssize_t nn=recv(fd,buf+bufi,conbufnn-bufi,0);
			if(nn==0){//closed
				delete this;
				return;
			}
			if(nn<0){//error
				if(errno==EAGAIN||errno==EWOULDBLOCK){
					io_request_read();
					return;
				}else if(errno==ECONNRESET){
					delete this;
					return;
				}
				printf("\n\n%s:%d ",__FILE__,__LINE__);perror("readbuf");
				stats.errors++;
				delete this;
				return;
			}
			bufnn+=(unsigned)nn;
			stats.input+=(unsigned)nn;
		}
//		printf("%s\n",buf);
		while(bufi<bufnn){
			bufi++;
			const char c=*bufp++;
			switch(state){
			case method:
				if(c==' '){
					state=uri;
					pth=bufp;
					qs=0;
				}
				break;
			case uri:
				if(c==' '){
					state=protocol;
					*(bufp-1)=0;
//					printf("%s\n",pth);
					urldecode(pth);
//					printf("%s\n",pth);
				}else if(c=='?'){
					state=query;
					qs=bufp;
					*(bufp-1)=0;
				}
				break;
			case query:
				if(c==' '){
					state=protocol;
					*(bufp-1)=0;
					urldecode(qs);
				}
				break;
			case protocol:
				if(c=='\n'){
					hdrs.clear();
					hdrk=bufp;
					state=header_key;
				}
				break;
			case header_key:
				if(c=='\n'){// content or done parsing
					const char*content_length_str=hdrs["content-length"];
					if(content_length_str){
						content_len=(size_t)atoll(content_length_str);
						const char*content_type=hdrs["content-type"];
						if(content_type&&strstr(content_type,"file")){// file upload
//							printf("uploading file: %s   size: %s\n",pth+1,content_length_str);
							const mode_t mod{0664};
							char buf[255];
							snprintf(buf,sizeof buf,"upload/%s",pth+1);
							upload_fd=open(buf,O_CREAT|O_WRONLY|O_TRUNC,mod);
							if(upload_fd<0){
								perror("while creating file for upload");
								throw"err";
							}
							const char*s=hdrs["expect"];
							if(s&&!strcmp(s,"100-continue")){
//								printf("client expects 100 continue before sending post\n");
								io_send(fd,"HTTP/1.1 100\r\n\r\n",16,true);
								state=upload;
								break;
							}
							const size_t chars_left_in_buffer=bufnn-bufi;
							if(chars_left_in_buffer==0){
								state=upload;
								break;
							}
							if(chars_left_in_buffer>=content_len){
//								printf(" upload in buffer\n");
								const ssize_t nn=write(upload_fd,bufp,(size_t)content_len);
								if(nn<0){
									perror("while writing upload to file");
									throw"err";
								}
								stats.input+=(size_t)nn;
								if((size_t)nn!=content_len){
									throw"incomplete upload";
								}
								if(::close(upload_fd)<0){
									perror("while closing file");
								}
								io_send(fd,"HTTP/1.1 200\r\n\r\n",16,true);
								bufp+=content_len;
								bufi+=content_len;
								state=next_request;
								break;
							}
							const ssize_t nn=write(fd,bufp,chars_left_in_buffer);
							if(nn<0){
								perror("while writing upload to file2");
								throw"err";
							}
							content_pos=(size_t)nn;
							state=upload;
							break;
						}
						// posted content
						delete[]content;
						content=new char[content_len+1];// extra char for end-of-string
						const size_t chars_left_in_buffer=bufnn-bufi;
						if(chars_left_in_buffer>=content_len){
							memcpy(content,bufp,(size_t)content_len);
							*(content+content_len)=0;
							bufp+=content_len;
							bufi+=content_len;
						}else{
							memcpy(content,bufp,chars_left_in_buffer);
							content_pos=chars_left_in_buffer;
							state=read_content;
							const char*s=hdrs["expect"];
							if(s&&!strcmp(s,"100-continue")){
								io_send(fd,"HTTP/1.1 100\r\n\r\n",16,true);
							}
							state=read_content;
							break;
						}
					}else{
						content=nullptr;
					}
					process();
					break;
				}else if(c==':'){
					*(bufp-1)=0;
					hdrv=bufp;
					state=header_value;
				}
				break;
			case header_value:
				if(c=='\n'){
					*(bufp-1)=0;
					hdrk=strtrm(hdrk,hdrv-2);
					strlwr(hdrk);
					hdrv=strtrm(hdrv,bufp-2);
					hdrs.put(hdrk,hdrv);
					hdrk=bufp;
					state=header_key;
				}
				break;
			case resume_send_file:
			case read_content:
			case upload:
			case next_request:
				throw"illegalstate";
			}
			if(state==upload){
				break;
			}else if(state==next_request){
				state=method;
				break;
			}
		}
		if(state==method){
			stats.requests++;
			const char*str=hdrs["connection"];
//			if(str&&strcmp("close",str)){
////				printf("connection close\n");
//				delete this;
//				return;
//			}else
			if(!str||strcasecmp("keep-alive",str)){
//				printf("connection close\n");
				delete this;
				return;
			}
		}
//		if(bufi==bufnn){// not finished parsing request
//			io_request_read();
//			return;
//		}
	}}
private:
	void process(){
		const char*path=pth+1;
		xwriter x=xwriter(fd);
		if(!*path&&qs){
			stats.widgets++;
			const char*cookie=hdrs["cookie"];
			const char*session_id;
			if(cookie&&strstr(cookie,"i=")){//? parse cookie
				session_id=cookie+sizeof "i="-1;
			}else{
				session_id=nullptr;
			}
			session*ses;
			if(!session_id){
				// create session
				//"Fri, 31 Dec 1999 23:59:59 GMT"
				time_t timer=time(NULL);
				struct tm*tm_info=gmtime(&timer);
				char*sid=(char*)malloc(24);
				//						 20150411--225519-ieu44d
				strftime(sid,size_t(24),"%Y%m%d-%H%M%S-",tm_info);
				char*sid_ptr=sid+16;
				for(int i=0;i<7;i++){
					*sid_ptr++='a'+random()%26;
				}
				*sid_ptr=0;
//				printf(" * creating session %s\n",session_id);
				ses=new session(sid);
				sessions.all.put(sid,ses,false);
				x.send_session_id_at_next_opportunity(sid);
			}else{
				ses=sessions.all[session_id];
				if(!ses){// session not found, reload
//					printf(" * session not found, recreating: %s\n",session_id);
					char*sid=(char*)malloc(24);
//					if(strlen(session_id)>23)throw"cookielen";
					strncpy(sid,session_id,24);
	//				printf(" * creating session %s\n",session_id);
					ses=new session(sid);
					sessions.all.put(sid,ses,false);
				}
			}
			widget*o=ses->get_widget(qs);
			if(!o){
//				printf(" * widget not found in session, creating  %s\n",qs);
				o=widgetget(qs);
				const size_t key_len=strlen(qs);
				char*key=(char*)malloc(key_len+1);
				memcpy(key,qs,key_len+1);
				ses->put_widget(key,o);
			}
			if(content){
//				printf(" * content:\n%s\n",content);
				o->on_content(x,content,content_len);
				delete[]content;
				content=nullptr;
				state=method;
				return;
			}else{
				o->to(x);
				state=method;
				return;
			}
		}
		if(!*path){
			stats.cache++;
			homepage->to(x);
			state=method;
			return;
		}
		if(strstr(path,"..")){
			x.reply_http(403,"path contains ..");
			state=method;
			return;
		}
		stats.files++;
		struct stat fdstat;
		if(stat(path,&fdstat)){
			x.reply_http(404,"not found");
			state=method;
			return;
		}
		if(S_ISDIR(fdstat.st_mode)){
			x.reply_http(403,"path is directory");
			state=method;
			return;
		}
		const struct tm*tm=gmtime(&fdstat.st_mtime);
		char lastmod[64];
		//"Fri, 31 Dec 1999 23:59:59 GMT"
		strftime(lastmod,sizeof lastmod,"%a, %d %b %y %H:%M:%S %Z",tm);
		const char*lastmodstr=hdrs["if-modified-since"];
		if(lastmodstr&&!strcmp(lastmodstr,lastmod)){
			const char*hdr="HTTP/1.1 304\r\n\r\n";
			const size_t hdrnn=strlen(hdr);
			io_send(fd,hdr,hdrnn,true);
			state=method;
			return;
		}
		file_fd=open(path,O_RDONLY);
		if(file_fd==-1){
			x.reply_http(404,"cannot open");
			state=method;
			return;
		}
		file_pos=0;
		file_len=size_t(fdstat.st_size);
		const char*range=hdrs["range"];
		char bb[K];
		int bb_len;
		if(range&&*range){
			off_t rs=0;
			if(EOF==sscanf(range,"bytes=%zu",&rs)){
				stats.errors++;
				printf("\n\n%s:%d ",__FILE__,__LINE__);perror("scanrange");
				throw"errrorscanning";
			}
			file_pos=rs;
			const size_t e=file_len;
			file_len-=(size_t)rs;
			bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 206\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",lastmod,file_len,rs,e,e);
		}else{
			// Connection: Keep-Alive\r\n for apache-bench
			bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 200\r\nConnection: Keep-Alive\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\n\r\n",lastmod,file_len);
		}
		if(bb_len<0)throw"err";
		io_send(fd,bb,(size_t)bb_len,true);
		const ssize_t nn=sendfile(fd,file_fd,&file_pos,file_len);
		if(nn<0){
			if(errno==EPIPE||errno==ECONNRESET){
				stats.brkp++;
				throw"brk";
			}
			stats.errors++;
			printf("\n\n%s:%d ",__FILE__,__LINE__);perror("sendfile");
			throw"err";
		}
		stats.output+=size_t(nn);
		file_len-=size_t(nn);
		if(file_len!=0){
			state=resume_send_file;
			io_request_write();
			return;
		}
		::close(file_fd);
		state=method;
	}
};
static sock server_socket;
static void*thdwatchrun(void*arg){
	if(arg)
		puts((const char*)arg);
	stats.printhdr(stdout);
	while(1){
		int n=10;
		while(n--){
			const int sleep=100000;
			usleep(sleep);
			stats.ms+=sleep/1000;//? not really
			stats.print(stdout);
		}
		fprintf(stdout,"\n");
	}
	return nullptr;
}
static void sigexit(int i){
	puts("exiting");
	delete homepage;
//	if(shutdown(server_socket.fd,SHUT_RDWR))perror("shutdown");
	server_socket.close();
//	close(server_socket.fd);
	signal(SIGINT,SIG_DFL);
	kill(getpid(),SIGINT);
//	exit(i);
}
int main(int argc,char**argv){
	signal(SIGINT,sigexit);
	printf("%s on port %d\n",APP,port);

	char buf[4*K];
	// Connection: Keep-Alive for apachebench
	snprintf(buf,sizeof buf,"HTTP/1.1 200\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\n\r\n%s",strlen(APP),APP);
	homepage=new doc(buf);

	struct sockaddr_in srv;
	const ssize_t srvsz=sizeof(srv);
	bzero(&srv,srvsz);
	srv.sin_family=AF_INET;
	srv.sin_addr.s_addr=INADDR_ANY;
	srv.sin_port=htons(port);
	if((server_socket.fd=socket(AF_INET,SOCK_STREAM,0))==-1){perror("socket");exit(1);}
	if(bind(server_socket.fd,(struct sockaddr*)&srv,srvsz)){perror("bind");exit(2);}
	if(listen(server_socket.fd,nclients)==-1){perror("listen");exit(3);}
	epollfd=epoll_create(nclients);
	if(!epollfd){perror("epollcreate");exit(4);}
	struct epoll_event ev;
	ev.events=EPOLLIN;
	ev.data.ptr=&server_socket;
	if(epoll_ctl(epollfd,EPOLL_CTL_ADD,server_socket.fd,&ev)<0){perror("epolladd");exit(5);}
	struct epoll_event events[nclients];
	const bool watch_thread=argc>1;
	pthread_t thdwatch;
	if(watch_thread)if(pthread_create(&thdwatch,nullptr,&thdwatchrun,nullptr)){perror("threadcreate");exit(6);}
	while(true){
		//printf(" epoll_wait\n");
		const int nn=epoll_wait(epollfd,events,nclients,-1);
		//printf(" epoll_wait returned %d\n",nn);
		if(nn==0){
			puts("epoll 0");
			continue;
		}
		if(nn==-1){
			perror("epollwait");
			continue;
		}
		for(int i=0;i<nn;i++){
			sock*c=(sock*)events[i].data.ptr;
			if(c->fd==server_socket.fd){// new connection
				stats.accepts++;
				const int fda=accept(server_socket.fd,0,0);
				if(fda==-1){
					perror("accept");
					puts("accept");
					continue;
//					exit(8);
				}
				int opts=fcntl(fda,F_GETFL);
				if(opts<0){
					perror("optget");
					puts("optget");
					continue;
//					exit(9);
				}
				opts|=O_NONBLOCK;
				if(fcntl(fda,F_SETFL,opts)){
					perror("optsetNONBLOCK");
					puts("optsetNONBLOCK");
					continue;
//					exit(10);
				}
				ev.data.ptr=new sock(fda);
//				ev.events=EPOLLIN|EPOLLRDHUP|EPOLLET;
//				ev.events=EPOLLIN|EPOLLRDHUP;
				ev.events=EPOLLIN;
				if(epoll_ctl(epollfd,EPOLL_CTL_ADD,fda,&ev)){
					perror("epolladd");
					puts("epolladd");
					continue;
//					exit(11);
				}
				int flag=1;
				if(setsockopt(fda,IPPROTO_TCP,TCP_NODELAY,(void*)&flag,sizeof(int))<0){
					perror("optsetTCP_NODELAY");
					puts("optsetTCP_NODELAY");
					exit(12);
				}
				continue;
			}
			try{
				c->run();
			}catch(const char*msg){
				if(msg!=exception_connection_reset_by_client){
					printf(" *** exception from %p : %s\n",(void*)c,msg);
				}
				delete c;
			}catch(...){
				printf(" *** exception from %p\n",(void*)c);
				delete c;
			}
		}
	}
}
//-- application
namespace web{
class hello:public widget{
	virtual void to(xwriter&x)override{
		x.reply_http(200,"hello world");
	}
};
class bye:public widget{
	virtual void to(xwriter&x)override{
		x.reply_http(200,"b y e");
	}
};
class notfound:public widget{
	virtual void to(xwriter&x)override{
		x.reply_http(404,"path not found");
	}
};
class typealine:public widget{
	virtual void to(xwriter&x)override{
		x.reply_http(200,"typealine");
	}
	virtual void on_content(xwriter&x,/*scan*/const char*content,const size_t content_len)override{
//		printf(" typealine received content: %s\n",content);
		x.reply_http(200,content,content_len);
	}
};
}
//-- generated
static widget*widgetget(const char*qs){
	if(!strcmp("hello",qs))
		return new web::hello();
	if(!strcmp("typealine",qs))
		return new web::typealine();
	if(!strcmp("bye",qs))
		return new web::bye();
	return new web::notfound();
}
