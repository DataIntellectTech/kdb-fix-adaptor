#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <map>
#include <functional>
#include <algorithm>
#include <stdexcept>

#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Values.h"
#include "quickfix/Utility.h"
#include "quickfix/Mutex.h"
#include "quickfix/FileStore.h"
#include "quickfix/ThreadedSocketAcceptor.h"
#include "quickfix/ThreadedSocketInitiator.h"
#include "quickfix/Log.h"
#include "quickfix/SessionSettings.h"

#include "kx/k.h"

#include "aquaq/FixEngine.h"
#include "aquaq/FixSession.h"

extern "C"
K send_new_order_single(K x)
{
    // TODO :: use this to test the valid untyped messages.
    FIX::Message message;
  
    message.getHeader().setField(FIX::FIELD::BeginString, "FIX.4.4");
    message.getHeader().setField(FIX::FIELD::SenderCompID, "SellSideID");
    message.getHeader().setField(FIX::FIELD::TargetCompID, "BuySideID");
    message.getHeader().setField(FIX::FIELD::MsgType, "D");

    message.setField(FIX::FIELD::ClOrdID, "SHD2015.04.04");
    message.setField(FIX::FIELD::Side, "1");
    message.setField(FIX::FIELD::TransactTime, "20150416-17:38:21");
    message.setField(FIX::FIELD::OrdType, "1");

    FIX::Session::sendToTarget(message, "SellSideID", "BuySideID", "0");	
}

extern "C"
K send_order_cancel(K x)
{
    // TODO :: use this to test the invalid untyped messages.
    FIX::Message message;

    message.getHeader().setField(FIX::FIELD::BeginString, "FIX.4.4");
    message.getHeader().setField(FIX::FIELD::SenderCompID, "SellSideID");
    message.getHeader().setField(FIX::FIELD::TargetCompID, "BuySideID");
    message.getHeader().setField(FIX::FIELD::MsgType, "F");

    message.setField(FIX::FIELD::OrigClOrdID, "123");
    message.setField(FIX::FIELD::ClOrdID, "312");
    message.setField(FIX::FIELD::Symbol, "LNUX");
    message.setField(FIX::FIELD::Side, "S");
    message.setField(FIX::FIELD::Text, "Cancel My Order!");

    FIX::Session::sendToTarget(message, "SellSideID", "BuySideID", "0");

    return (K) 0;
}

static void tnprintf(char *buffer, size_t size, char *str, time_t time)
{
    struct tm* timeinfo = localtime(&time);
    timeinfo->tm_hour -= 1;
    strftime(buffer, size, str, timeinfo);
}

char *convert_to_string(K x)
{
    static char buffer[4096];

    switch (xt) {
        case -1:  snprintf(buffer, sizeof(buffer), "%d", x->g); break;
        case -4:  snprintf(buffer, sizeof(buffer), "0x%02x", x->g); break;
        case -5:  snprintf(buffer, sizeof(buffer), "%d", x->h); break;
        case -6:  snprintf(buffer, sizeof(buffer), "%d", x->i); break;
        case -7:  snprintf(buffer, sizeof(buffer), "%lld", x->j); break;
        case -8:  snprintf(buffer, sizeof(buffer), "%f", x->e); break;
        case -9:  snprintf(buffer, sizeof(buffer), "%f", x->f); break;
        case -10: snprintf(buffer, sizeof(buffer), "%c", x->g); break;
        case -11: snprintf(buffer, sizeof(buffer), "%s", x->s); break;
        case -13: snprintf(buffer, sizeof(buffer), "%04d.%02d", (x->i)/12+2000, (x->i)%12+1); break;
        case -17: tnprintf(buffer, sizeof(buffer), "-17/%H:%M", (x->i) * 60); break;
        case -18: tnprintf(buffer, sizeof(buffer), "-18/%H:%M:%S", x->i); break;
        case -19: tnprintf(buffer, sizeof(buffer), "-19/%H:%M:%S", (x->i)/1000); break;
        default: snprintf(buffer, sizeof(buffer), "unknown type");
    }
 
    return buffer;
}

static int FieldFromString(char *str)
{
    if (0 == strcmp("BeginString", str)) {
        return FIX::FIELD::BeginString;
    } else if (0 == strcmp("SenderCompID", str)) {
        return FIX::FIELD::SenderCompID;
    } else if (0 == strcmp("TargetCompID", str)) {
        return FIX::FIELD::TargetCompID;
    } else if (0 == strcmp("MsgType", str)) {
        return FIX::FIELD::MsgType;
    } else if (0 == strcmp("ClOrdID", str)) {
        return FIX::FIELD::ClOrdID;
    } else if (0 == strcmp("OrigClOrdID", str)) {
        return FIX::FIELD::OrigClOrdID;
    } else if (0 == strcmp("Symbol", str)) {
        return FIX::FIELD::Symbol;
    } else if (0 == strcmp("Side", str)) {
        return FIX::FIELD::Side;
    } else if (0 == strcmp("Text", str)) {
        return FIX::FIELD::Text;
    }

    return 42;
}

extern "C"
K new_message(K sid, K x)
{
    auto session = reinterpret_cast<FixSession<FIX::ThreadedSocketInitiator>*>(sid->j); 

    std::cout << "begin string: " << session->id()->getBeginString() << std::endl;
    std::cout << "target id: "  << session->id()->getTargetCompID() << std::endl;
    std::cout << "sender id: " << session->id()->getSenderCompID() << std::endl;

    return (K) 0;
}

std::string readArgument(K obj, char *label, char * def)
{
    if (obj->t == 101) {
        return std::string(def);
    }

    K keys = kK(obj)[0];

    if (keys->n == 0 || keys->t != 11) {
        return std::string(def);
    }

    K values = kK(obj)[1];

    for (int i = 0; i < keys->n; i++) {
        S key = kS(keys)[i];
  
        if (0 == strcmp(key, label)) {
            if (values->t == 0) {
                return std::string(convert_to_string(kK(values)[i]));
            } else {
                return std::string(kS(values)[i]);
            }
        }
    }

    return std::string(def); 
}

extern "C"
K new_initiator(K args) {
    FixSession<FIX::ThreadedSocketInitiator>::Builder builder;
    
    builder.setVersion(readArgument(args, "version", "FIX.4.4"))
           .setTargetId(readArgument(args, "targetid", "BuySideID"))
           .setSourceId(readArgument(args, "sourceid", "SellSideID"))
           .setQualifier(readArgument(args, "qualifier", "0"))
           .setPort(readArgument(args, "port", "7070"))
           .setHost(readArgument(args, "host", "127.0.0.1"))
           .setType("initiator");

    try {
        auto session = builder.build();
        session->start();
        return kj((intptr_t) session);
    } catch (std::exception& ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
    }
}

extern "C"
K new_acceptor(K args) {
    FixSession<FIX::ThreadedSocketAcceptor>::Builder builder;
 
    builder.setVersion(readArgument(args, "version", "FIX.4.4"))
           .setTargetId(readArgument(args, "targetid", "SellSideID"))
           .setSourceId(readArgument(args, "sourceid", "BuySideID"))
           .setPort(readArgument(args, "port", "7070"))
           .setType("acceptor");

    try {
        auto session = builder.build();
        session->start();
        return kj((intptr_t) session);
    } catch (std::exception& ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
    }
}
