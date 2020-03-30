#include <iostream>

#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Values.h>
#include <quickfix/Utility.h>
#include <quickfix/Mutex.h>
#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/FieldMap.h>
#include <quickfix/ThreadedSocketAcceptor.h>
#include <quickfix/ThreadedSocketInitiator.h>
#include <quickfix/Log.h>
#include <quickfix/SessionSettings.h>

#include "socketpair.h"
#include <kx/k.h>

#include <config.h>
#include <string.h>
#include <pugixml.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <algorithm>


std::string typeconvert(std::string s);
std::string typedtostring(K x);
void CreateTypeMap(char*);
K convertmsgtype(std::string field, std::string type);
std::unordered_map<int,std::string> typemap;
std::unordered_map<int,std::string> sessionmap;

int sockets[2];
int created=0;

FIX::ThreadedSocketConnection *allsessions=nullptr;
FIX::ThreadedSocketInitiator *initiators=nullptr;
FIX::ThreadedSocketAcceptor *acceptors=nullptr;

class FixEngineApplication : public FIX::Application
{
    public:
    void onCreate(const FIX::SessionID& sessionID);
    void onLogon(const FIX::SessionID& sessionID);
    void onLogout(const FIX::SessionID& sessionID);
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID);
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
        EXCEPT (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon);
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) EXCEPT (FIX::DoNotSend);
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
        EXCEPT (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType);
};

static void FillFromIterators(FIX::FieldMap::Fields::const_iterator begin, FIX::FieldMap::Fields::const_iterator end, K* keys, K* values)
{
    for (auto it = begin; it != end; it++) {
        J tag = (J) it->getTag();
        auto str = it->getString().c_str();

        std::unordered_map<int, std::string>::const_iterator found = typemap.find(tag);
        ja(keys, &tag);

        if(55==tag){
            jk(values, ks(const_cast<char *>(str)));
	}
	else{
	    jk(values, convertmsgtype(str, found->second));
	};
    }
}

static K ConvertToDictionary(const FIX::Message& message)
{
    K keys = ktn(KJ, 0);
    K values = ktn(0, 0);

    auto header = message.getHeader();
    auto trailer = message.getTrailer();

    FillFromIterators(header.begin(), header.end(), &keys, &values);
    FillFromIterators(message.begin(), message.end(), &keys, &values);
    FillFromIterators(trailer.begin(), trailer.end(), &keys, &values);
    
    return xD(keys, values);
}

static void WriteToSocket(K x)
{
    static char buffer[8192];

    K bytes = b9(-1, x);
    r0(x);

    memcpy(&buffer, (char*) &bytes->n, sizeof(J));
    memcpy(&buffer[sizeof(J)], kG(bytes), (size_t) bytes->n);
 
    send(sockets[0], buffer, (int) sizeof(J) + bytes->n, 0);
    r0(bytes);
}

void FixEngineApplication::onCreate(const FIX::SessionID& sessionID)
{

}

void FixEngineApplication::onLogon(const FIX::SessionID& sessionID)
{

}

void FixEngineApplication::onLogout(const FIX::SessionID& sessionID)
{

}

void FixEngineApplication::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID)
{

}

void FixEngineApplication::toApp(FIX::Message& message, const FIX::SessionID& sessionID) EXCEPT (FIX::DoNotSend)
{

}

void FixEngineApplication::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) EXCEPT (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon)
{
    WriteToSocket(ConvertToDictionary(message));
}

void FixEngineApplication::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) EXCEPT (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType)
{
    WriteToSocket(ConvertToDictionary(message));
}

extern "C"
K SendMessageDict(K x)
{
    
    if (x->t != 99 || kK(x)[0]->t != 7 || kK(x)[1]->t != 0)
        return krr((S) "type");

    K keys = kK(x)[0];
    K values = kK(x)[1];

    auto beginString = std::string("");
    auto senderCompId = std::string("");
    auto targetCompId = std::string("");
    auto sessionQualifier = std::string("");
 
    FIX::Message message;
    FIX::Header &header = message.getHeader();

    for (int i = 0; i < keys->n; i++) {
        int tag = kJ(keys)[i];

        auto rep = typedtostring(kK(values)[i]);
	
        if (tag == 35 || tag == 8 || tag == 49 || tag == 56) {
            header.setField(tag, rep);
        } else {
            message.setField(tag, rep);
        }
	
    }

    try {
        FIX::Session::sendToTarget(message);
    } catch(FIX::SessionNotFound& ex) {
        std::cout << "unable to send message - session not found" << std::endl;
    }
    
    return (K) 0;
}

static inline void ReadBytes(int numbytes, char (*buf)[4096])
{
    int total = 0;
    do { total += recv(sockets[1], &buf[total], (int) (numbytes - total), 0); } while (total < numbytes);
}

extern "C"
K RecieveData(I x)
{
    static char buf[4096];
    J size = 0;

    ReadBytes(sizeof(J), &buf);
    memcpy(&size, buf, sizeof(J));
   
    K bytes = ktn(KG, size);

    ReadBytes(size, &buf);
    memcpy(kG(bytes), &buf, (size_t) size);
    K r = k(0, (char *)".fix.onrecv", d9(bytes), (K) 0);
    r0(bytes);

    if (r != 0) { r0(r); }

    return (K) 0;
}

template<typename T>
K CreateThreadedSocket(K x,T* &y) {
    if (x->t != -11) {
        return krr((S) "type");
    }

    std::string settingsPath = "settings.ini";

    settingsPath = std::string(x->s);
    settingsPath.erase(std::remove(settingsPath.begin(), settingsPath.end(), ':'), settingsPath.end());

    auto settings = new FIX::SessionSettings(settingsPath);
    auto application = new FixEngineApplication;
    auto store = new FIX::FileStoreFactory(*settings);
    auto log = new FIX::FileLogFactory(*settings);
    
    T *socket = nullptr;
    socket = new T(*application, *store, *settings, *log);
    y=&(*socket);
    socket->start();

    return (K) 0;
}


K GetSessionDetails(K x) {
   int cnt=0;
   K names=ktn(KS,0);
   K ptrs=ktn(KJ,0);
   K acceptor=ktn(KI,0);
   K isloggedon=ktn(KI,0);
   K isenabled=ktn(KI,0);
   K nextsenderseqnum=ktn(KI,0);
   K nexttargetseqnum=ktn(KI,0);
   K keys=ktn(KS,0);
    js(&keys,ss((S)"name"));
    js(&keys,ss((S)"ptr"));
    js(&keys,ss((S)"acceptor"));
    js(&keys,ss((S)"loggedon"));
    js(&keys,ss((S)"enabled"));
    js(&keys,ss((S)"nextsenderseqnum"));
    js(&keys,ss((S)"nexttargetseqnum"));
    if((nullptr!=initiators)) {
        std::set < FIX::SessionID >  sessions = initiators->getSessions();
        std::set < FIX::SessionID >  ::iterator i;
        for (i = sessions.begin(); i != sessions.end(); ++i) {
            FIX::Session *yy = initiators->getSession(*i);
            cnt = cnt + 1;
            std::string p = yy->getSessionID().toString();
            int isacceptor = 0;
            int loggedon = (int) (yy->isLoggedOn());
            int enabled = (int) (yy->isEnabled());
            int nexttarget = (int) (yy->getExpectedSenderNum());
            int nextsender = (int) (yy->getExpectedSenderNum());
            long ptr = (long) yy;
            js(&names, ss((S) (p.c_str())));
            ja(&acceptor, &isacceptor);
            ja(&isloggedon, &loggedon);
            ja(&isenabled, &enabled);
            ja(&nexttargetseqnum, &nextsender);
            ja(&ptrs, &ptr);
            sessionmap.insert({ptr,yy->getSessionID().toString()});
        }
    }
    if ((nullptr!=acceptors))
        {  std::set < FIX::SessionID > asessions = acceptors->getSessions();
            std::set < FIX::SessionID >  ::iterator j;
            for (j = asessions.begin(); j != asessions.end(); ++j) {
                FIX::Session *ayy = acceptors->getSession(*j);
                cnt = cnt + 1;
                std::string p = ayy->getSessionID().toString();
                int isacceptor = 1;
                int loggedon = (int) (ayy->isLoggedOn());
                int enabled = (int) (ayy->isEnabled());
                int nexttarget = (int) (ayy->getExpectedSenderNum());
                int nextsender = (int) (ayy->getExpectedSenderNum());
                long ptr = (long) ayy;
                js(&names, ss((S) (p.c_str())));
                ja(&acceptor, &isacceptor);
                ja(&isloggedon, &loggedon);
                ja(&isenabled, &enabled);
                ja(&nextsenderseqnum, &nexttarget);
                ja(&nexttargetseqnum, &nextsender);
                ja(&ptrs, &ptr);
                sessionmap.insert({ptr,ayy->getSessionID().toString()});
            }
        }
   K vals=knk(7,names,ptrs,acceptor,isloggedon,isenabled,nextsenderseqnum,nexttargetseqnum);


     return r1(xT(xD(keys,vals))); 
}


extern "C"
K Create(K x, K y) {
   K ret;
   int i=0;
    if(-11 != x->t){
        return krr((S) "type");
    }

    K z = ks((S) "sessions/sample.ini");

    if (-11 == y->t){
	r0(z);
	z = ks((S) y->s);
    }
    else{
        std::cout << "Defaulting to sample.ini config" << std::endl;
    }
     
    if(created>0)
    {
        return krr((S)"created");
    }

    dumb_socketpair(sockets, 0);
    sd1(sockets[1], RecieveData);
  try{
     ret = CreateThreadedSocket<FIX::ThreadedSocketInitiator>(z,initiators);
  }
  catch(const std::exception& e)
  {      i=i+1;
	  std::cout << e.what() << std::endl;
  }
  
   try{
     ret = CreateThreadedSocket<FIX::ThreadedSocketAcceptor>(z,acceptors);
   } catch(const std::exception& e)
   {
	i=i+1;   
    std::cout << e.what() << std::endl;
   }
    r0(z);

    created=1;

    return (K) ki(i);
}

extern "C"
K OnRecv(K x) { return (K) 0; }

extern "C"
K Version(K x){ 
    K keys = ktn(KS, 4);
    K values = ktn(KS, 4);

    kS(keys)[0] = ss((S) "release");
    kS(keys)[1] = ss((S) "quickfix");
    kS(keys)[2] = ss((S) "os"); 
    kS(keys)[3] = ss((S) "kx");

    kS(values)[0] = ss((S) BUILD_PROJECT_VERSION);
    kS(values)[1] = ss((S) BUILD_QUICKFIX_VERSION);
    kS(values)[2] = ss((S) BUILD_OPERATING_SYSTEM);
    kS(values)[3] = ss((S) BUILD_KX_VER);

    return xD(keys, values);
}

extern "C"
K LoadLibrary(K x)
{  

    char *TYPEDEFFILE = (char*) "./spec/FIX42.xml";	
    
    if( 10==x->t)
    {
       TYPEDEFFILE=(char*)kC(x)	;
       TYPEDEFFILE[x->n]='\0';
    }
    
    printf("████████████████████████████████████████████████████\n");
    printf(" release » %-5s                                     \n", BUILD_PROJECT_VERSION);
    printf(" quickfix » %-5s                                    \n", BUILD_QUICKFIX_VERSION);
    printf(" os » %-5s                                          \n", BUILD_OPERATING_SYSTEM);
    printf(" arch » %-5s                                        \n", BUILD_PROCESSOR);
    printf(" git commit » %-5s                                  \n", BUILD_GIT_SHA1);
    printf(" git commit datetime » %-5s                         \n", BUILD_GIT_DATE);
    printf(" build datetime » %-5s                              \n", BUILD_DATE);
    printf(" kdb compatibility » %s.x                           \n", BUILD_KX_VER);
    printf(" compiler flags » %-5s                              \n", BUILD_COMPILER_FLAGS);
    printf(" typemap default file » -%5s                        \n", TYPEDEFFILE);
    printf("████████████████████████████████████████████████████\n");

    K keys = ktn(KS, 5);
    K values = ktn(0, 5);

    kS(keys)[0] = ss((S) "send");
    kS(keys)[1] = ss((S) "onrecv");
    kS(keys)[2] = ss((S) "create");
    kS(keys)[3] = ss((S) "version");
    kS(keys)[4] = ss((S) "sessiondetails");

    kK(values)[0] = dl((void *) SendMessageDict, 1);
    kK(values)[1] = dl((void *) OnRecv, 1);
    kK(values)[2] = dl((void *) Create, 2);
    kK(values)[3] = dl((void *) Version, 1);
    kK(values)[4] = dl((void *) GetSessionDetails, 1);
    try{CreateTypeMap(TYPEDEFFILE);}
     catch (...)
    {
    return krr((S) "typemapfile");
    };

    return xD(keys, values);
}

void CreateTypeMap(char * TYPEFILE )
{  
    pugi::xml_document doc;
    if(!doc.load_file(TYPEFILE)) throw std::runtime_error("XML could not be loaded");
    pugi::xml_node fields = doc.child("fix").child("fields");
    
    for(pugi::xml_node field = fields.child("field"); field; field = field.next_sibling("field"))
    {
        int value = field.attribute("number").as_int();		
        std::string fieldattr = typeconvert(field.attribute("type").value());
        typemap.insert({value, fieldattr});
    }   
}

std::string typeconvert(std::string s)
{
    std::unordered_map<std::string,std::string> typemapconvert = {
        {"STRING","STRING"},
        {"MULTIPLEVALUESTRING","STRING"},
        {"PRICE","FLOAT"},
        {"CHAR","CHAR"},
        {"INT","INT"},
        {"AMT","FLOAT"},
        {"CURRENCY","FLOAT"},
        {"QTY","FLOAT"},
        {"EXCHANGE","SYM"},
        {"UTCTIMESTAMP","TIMESTAMP"},
        {"BOOLEAN","BOOLEAN"},
        {"LOCALMKTDATE","DATE"},
        {"DATA","STRING"},
        {"LENGTH","FLOAT"},
        {"FLOAT","FLOAT"},
        {"PRICEOFFSET","FLOAT"},
        {"MONTHYEAR","STRING"},
        {"DAYOFMONTH","STRING"},
        {"UTCDATE","DATE"},
        {"UTCTIMEONLY","TIME"},
        {"COUNTRY","STRING"},
        {"DATE", "STRING"},
        {"EXCHANGE", "SYM"},
        {"LANGUAGE", "STRING"},
        {"MULTIPLECHARVALUE", "STRING"},
        {"MULTIPLESTRINGVALUE", "STRING"},
        {"NUMINGROUP", "INT"},
        {"PERCENTAGE", "FLOAT"},
        {"PRICEOFFSET", "FLOAT"},
        {"SEQNUM", "INT"},
        {"TIME", "STRING"},
        {"UTCDATE", "DATE"},
        {"UTCTIMEONLY", "STRING"},
    };

    std::unordered_map<std::string,std::string>::const_iterator found = typemapconvert.find(s);
    
    if(found != typemapconvert.end() ){
	    return found->second;
    }
    else{
	    return "STRING";
    }
}



struct Tm : std::tm {
	  int tm_usecs;

    Tm(const int year, const int month, const int mday, const int hour,
       const int min, const int sec, const int usecs, const int isDST = -1)
          : tm_usecs{usecs} {
	      tm_year = year - 1900;
     	      tm_mon = month - 1;
	      tm_mday = mday;
              tm_hour = hour;
	      tm_min = min;
              tm_sec = sec;
	      tm_isdst = isDST;
            }

    template <typename Clock_t = std::chrono::high_resolution_clock> 
    auto to_time_point() -> typename Clock_t::time_point {
        auto time_c = mktime(this);
        return Clock_t::from_time_t(time_c);
    }
};

K pu(I x){
    return k(0, (S) "`timestamp$", kz(x/8.64e4 - 10957), (K)0);
};

K strtotemporal(std::string datestring){

    int year, month, day, hour, minute, second, ms;

    sscanf(datestring.c_str(), 
        "%4d%2d%2d-%2d:%2d:%2d.%3d", 
        &year, 
        &month, 
        &day,
        &hour,
        &minute,
        &second,
        &ms);

    auto tp_micro = Tm(year, month, day, hour, minute, second, ms).to_time_point(); 
    K tp = pu(std::chrono::high_resolution_clock::to_time_t(tp_micro)); // ms from Unix epoch 
    auto time = tp->j + ms*1e6;

    return kj(time);
}   

int strtodate(std::string date){
    int year, month, day;
    sscanf(date.c_str(),
        "%4d%2d%2d",
	&year,
	&month,
	&day);
    struct std::tm a = {0,0,0,day,month-1,year-1900};
    struct std::tm b = {0,0,0,1,0,100};
    std::time_t x = std::mktime(&a);
    std::time_t y = std::mktime(&b);
    int difference = std::difftime(x,y) / (60*60*24);
    return difference;
}

int strtotime(std::string time){

    int hour, minute, second;
    sscanf(time.c_str(),
	"%2d:%2d:%2d",
	&hour,
	&minute,
	&second);
    struct std::tm a = {second,minute,hour,1,0,0};
    struct std::tm b = {0,0,0,1,0,0};
    std::time_t x = std::mktime(&a);
    std::time_t y = std::mktime(&b);
    int difference = std::difftime(x,y) *1e3;
    return difference;
}

K convertmsgtype(std::string field, std::string type)
{

    if("FLOAT"==type){
        return kf(std::stof(field));
    }
    else if("STRING"==type){
	return kp(const_cast<char *>(field.c_str()));
    }
    else if("INT"==type){
        return ki(std::stoi(field));
    }
    else if("CHAR"==type){
	char *c = &field[0u];
	int d = c[0];
	return kc(d);
    }
    else if("BOOLEAN"==type){
        if("Y"==field){
            return kb(1);
        }
	else{
            return kb(0);
	}	 
    }
    else if("TIMESTAMP"==type){
	auto ts = strtotemporal(field);
	return ktj(-KP,ts->j);
    }
    else if("DATE"==type){
	auto date = strtodate(field);    
        return kd(date);
    }
    else if("TIME"==type){
	auto time = strtotime(field);
        return kt(time);
    }
    else {
	return kp(const_cast<char *>(field.c_str()));
    }

}

std::string typedtostring(K x){
     
    if(-1==x->t){
        if(1==x->g){
            x = kp(const_cast<char *>("Y"));
	}
	else{
            x = kp(const_cast<char *>("N"));
	}
    }
    else if(x->t <= -2 && x->t >=-11){
	x=k(0, (S) "string ", r1(x), (K)0);
    }
    else if(-12==x->t){
        long kdbtime = x->j;
        long unixtime = (kdbtime/8.64e13+10957)*8.64e4;
        time_t seconds(unixtime);
        tm *p = gmtime(&seconds);

        char buffer[80]; // make a power of 2
        strftime(buffer, 80, "%Y%m%d-%T.000", p);  

        std::string timestr = std::to_string(kdbtime);
			   
        buffer[18] = timestr[9];
        buffer[19] = timestr[10];
        buffer[20] = timestr[11];
	x = kp(buffer);
    }
    else if(-14==x->t){
	x = k(0, (S) "{ssr[string x;\".\";\"\"]}", x, (K)0);
    }
    else if(-13==x->t){
	    x = k(0, (S) "{ssr[string x;\".\";\"\"]}", x, (K)0);
    }

    std::string rep("");
   
    for (int i = 0; i < x->n; i++) {
        rep += kC(x)[i];
    }

    return rep;
}

