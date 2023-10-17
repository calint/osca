#include "strb.h"

inline void strbi(strb*o){
	o->index=0;
}

inline size_t strbrem(strb*o){
	const long long remaining=(long long)((sizeof o->chars)-o->index);
	return remaining;
}

inline int strbp(strb*o,const char*str){
	const size_t remaining=strbrem(o);
	const int n=snprintf(o->chars+o->index,remaining,"%s",str);
	if(n<0)return-1;
	o->index+=n;
	const size_t remaining2=strbrem(o);
	if(remaining2<1)return-2;
//	printf("index:%zd  remaining:%zd  string:'%s'\n",o->index,remaining2,o->chars);
	return 0;
}
inline int strbfmtlng(strb*o,const long long n){
	char s[32];snprintf(s,sizeof s,"%lld",n);
	strbp(o,s);
	return 0;
}
int strbfmtbytes(strb*o,long long bytes){
	const long long kb=bytes>>10;
	if(kb==0){
		if(strbfmtlng(o,bytes))return-1;
		if(strbp(o," B"))return-2;
		return 0;
	}
	const long long mb=kb>>10;
	if(mb==0){
		if(strbfmtlng(o,kb))return-3;
		if(strbp(o," KB"))return-4;
		return 0;
	}
	if(strbfmtlng(o,mb))return-5;
	if(strbp(o," MB"))return-6;
	return 0;
}

/// clears buffer by setting index to 0 and first char to EOS
void strbclr(strb*o){
	o->index=0;
	o->chars[0]='\0';
}
