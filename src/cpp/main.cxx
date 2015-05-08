#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <map>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <cstdarg>
#include <ctime>

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

using namespace FIX;

static inline std::string fmt_time(char *str, time_t time)
{
    // Note :: This assumes that the kdb+ instance is set to the same time offset from
    // GMT as the machine is. If the time appears to be out of sync on the machines, then
    // this could be part of the problem.
    static char buffer[4096];
    struct tm *timeinfo = gmtime(&time);

    strftime(buffer, sizeof(buffer), str, timeinfo); 

    return std::string(buffer);
}

static inline std::string readAtom(K x, int index)
{
    static char buffer[4096];
 
    switch (xt) {
        case 1:  snprintf(buffer, sizeof(buffer), "%d",   kG(x)[index]); break;
        case 5:  snprintf(buffer, sizeof(buffer), "%d",   kH(x)[index]); break;
        case 6:  snprintf(buffer, sizeof(buffer), "%d",   kI(x)[index]); break;
        case 7:  snprintf(buffer, sizeof(buffer), "%lld", kJ(x)[index]); break;
        case 8:  snprintf(buffer, sizeof(buffer), "%f",   kE(x)[index]); break;
        case 9:  snprintf(buffer, sizeof(buffer), "%f",   kF(x)[index]); break;
        case 10: snprintf(buffer, sizeof(buffer), "%c",   kG(x)[index]); break;
        case 11: snprintf(buffer, sizeof(buffer), "%s",   kS(x)[index]); break;
        default: return "unknown type";
    }

    return std::string(buffer);
}

////
// Reads the argument from a kdb+ dictionary and attempts to return the value parsed as a string.
// If the label is not present in the dictionary, the default argument that is passed in as a
// parameter will be returned.
//
static std::string readArgument(K obj, const std::string& label, const std::string& defaultarg)
{
    // If we don't get a null type or a dictionary, just return the defaults. Do
    // the same if the keys of the dictionary are not symbols.
    if (obj->t != 99 || kK(obj)[0]->t != 11)
        return defaultarg;
 
    K keys = kK(obj)[0];
    K values = kK(obj)[1];

    for (int i = 0; i < keys->n; i++)
        if (0 == strcmp(kS(keys)[i], label.c_str()))
            return readAtom(values, i);

    return defaultarg;
}

extern "C"
K send_message(K args)
{
    if (args->t != 99 || kK(args)[0]->t != 7)
        return krr((S) "type");

    FIX::Message message;

    K keys = kK(args)[0];
    K values = kK(args)[1];
 
    auto beginString = std::string("");
    auto senderCompId = std::string("");
    auto targetCompId = std::string("");
    auto sessionQualifier = std::string("");

    for (int i = 0; i < keys->n; i++) {
        int tag = kJ(keys)[i];
        auto value = readAtom(values, i);
 
        if (tag == 35 || tag == 8 || tag == 49 || tag == 56) {
            message.getHeader().setField(tag, value); 
        } else {
            message.setField(tag, value);
        }

        if (8 == tag) {
            beginString = value;
        } else if (49 == tag) {
            senderCompId = value;
        } else if (56 == tag) {
            targetCompId = value;
        } 
    }
 
    FIX::Session::sendToTarget(message, senderCompId, targetCompId, sessionQualifier);
    return (K) 0;
}

K set_session_args(K args, FIX::Dictionary& session)
{
    if (kK(args)[0]->t != 11) { return krr((S) "type"); }

    K keys = kK(args)[0];
    K values = kK(args)[1];
 
    for (auto i = 0; i < keys->n; i++) {
        auto key = std::string(kS(keys)[i]);
        auto value = readAtom(values, i);
        session.setString(key, value);
    }

    return (K) 0;
}

Dictionary globals;

////
// Updates the globals with the arguments provided in the dictionary. If the
// arguments are invalid, they will be placed into the dictionary anyways and
// handled when an initiator or acceptor is first constructed.
//
extern "C"
K fix_set_defaults(K args) {
    if (args->t != 99) return krr((S) "type");
    return set_session_args(args, globals);
}

extern "C"
K fix_get_defaults(K args) {
    Dictionary defaults;

    auto settings = std::make_shared<SessionSettings>("config/acceptor-config.ini");
    auto& iniSettings = settings->get();

    defaults.merge(globals);
    defaults.merge(iniSettings);

    K keys = ktn(KS, 0);
    K values = ktn(KS, 0); 

    for (auto i = defaults.begin(); i != defaults.end(); i++) {
        js(&keys, (S) i->first.c_str());
        js(&values, (S) i->second.c_str());
    }
 
    return xD(keys, values);
}

template<typename T>
K create_thread(K args, const std::string& config)
{
    // Check for dictionary or identity function type (::) and throw a type error
    // if we get anything else.
    if (args->t != 99 && args->t != 101)
        return krr((S) "type");
 
    auto settings = new SessionSettings(config);
 
    // Get a copy of the default settings from the ini file and merge it with the
    // globals that have been set at runtime.
    Dictionary defaults;

    auto& iniSettings = settings->get();
    defaults.merge(globals);
    defaults.merge(iniSettings);
    settings->set(defaults);
 
    // Create a second dictionary to populate with session specific configuration.
    // If we are not passed an argument, we just leave it empty.
    Dictionary session;
    if (args->t != 101)
        set_session_args(args, session);
 
    // Now that we have all of the configuration available to us, we can pick through it
    // to find the session id. We don't check if the beginstring, sendercompid or targetcompid
    // exist in the dictionary as these should always be user defined and we want an exception
    // to be thrown.
    auto beginString = (defaults.has("BeginString")) ? defaults.getString("BeginString") : "BadBeginString";
    auto senderCompID = (defaults.has("SenderCompID")) ? defaults.getString("SenderCompID") : "BadSenderCompID";
    auto targetCompID = (defaults.has("TargetCompID")) ? defaults.getString("TargetCompID") : "BadTargetCompID";
    auto sessionQualifier = (defaults.has("SessionQualifier")) ? defaults.getString("SessionQualifier") : "";

    if (session.has("BeginString")) beginString = session.getString("BeginString");
    if (session.has("SenderCompID")) senderCompID = session.getString("SenderCompID");
    if (session.has("TargetCompID")) targetCompID = session.getString("TargetCompID");
    if (session.has("SessionQualifier")) sessionQualifier = session.getString("SessionQualifier");
 
    // We need to override the DataDictionary and AppDataDictionary to be consistent with the begin
    // string that the user has decided to use UNLESS they have explicitly set it in settings files.
    if (!session.has("DataDictionary") && !defaults.has("DataDictionary")) {
        session.setString("DataDictionary", "config/spec/" + beginString);
        session.setString("AppDataDictionary", "config/spec/" + beginString); 
    }
 
    // Now we can build the session id and store the session specific settings so that quickfix
    // is able to look it up again.
    auto sessionId = new SessionID(beginString, senderCompID, targetCompID, sessionQualifier);
    settings->set(*sessionId, session); 

    // Write the settings out to stdout before we finish creating the background
    // processing thread (which may fail if the config is incorrect). 
    std::cout << *settings << std::endl;
 
    // Start building the background thread. It will pretty much just be configuration
    // setting failures here, so we raise a config error to kdb+ and print the detailed
    // message to stderr.
    auto application = new FixEngineApplication;
    auto store = new FileStoreFactory(*settings);
    auto log = new ScreenLogFactory(*settings);
    auto thread = new T(*application, *store, *settings, *log);

    thread->start();

    return (K) 0;
}

////
// Connect to a FIX server as a new client. Dict should contain all the necessary information.
// Defaulting (e.g. of ports) can be done based on local config files -- this should be logged
// to std out so that it is clear from the log files which connection details are being used.
//
extern "C"
K fix_connect(K args)
{
    try {
        return create_thread<FIX::ThreadedSocketInitiator>(args, "config/initiator-config.ini");
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return (K) krr((S) "config");
    }
}

////
// Start listening on a server port for client connections. Like the connect function, anything 
// missing from the dict can be filled by configuration -- but the dictionary values should take
// precedence.
//
extern "C"
K fix_listen(K args)
{
    try {
        return create_thread<FIX::ThreadedSocketAcceptor>(args, "config/acceptor-config.ini");
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return (K) krr((S) "config");
    }
}

////
// Msgtype should be an event type defined in the configuration. Msgdict can be either ints!value
// or syms!values where the keys are fix tags or tag names. Validation should be performed on the
// message (correct types, presence of all required fields).
//
extern "C"
K fix_send(K args)
{
    try {
        return send_message(args);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return (K) krr((S) "config");
    }
}

////
// The default event handler that should be replaced by the users if they want to recieve FIX messages. The messages
// are delivered in a dictionary format where the keys are integers that map to the FIX tags and the values are kdb+
// types.
// 
extern "C"
K fix_onrecv(K args)
{
    if (args->t != 99) return krr((S) "type");
    std::cout << "default implementation of .fix.onrecv:{[dict] ... } (replace this handler to recieve messages or silence this message)" << std::endl;
    return (K) 0;
}

////
// Returns all of the active sessions that have been started since the library was loaded. The result
// is a list of dictionaries, with a dictionary containing information for a session.
//
extern "C"
K fix_get_sessions(K args)
{
    auto sessionIds = Session::getSessions();
    K list = ktn(0, 0);

    for (auto &sessionId : sessionIds) {
        auto session = Session::lookupSession(sessionId);

        K keys = ktn(KS, 0);
        K values = ktn(0, 0);
    
        js(&keys,   ss((S) "beginString"));
        jk(&values, ks((S) "beginString"));
 
        js(&keys,   ss((S) "senderCompID"));
        jk(&values, ks((S) "senderCompID"));

        js(&keys,   ss((S) "targetCompID"));
        jk(&values, ks((S) "targetCompID"));

        js(&keys,   ss((S) "sessionQualifier"));
        jk(&values, ks((S) "")); 

        js(&keys,   ss((S) "isInitiator"));        
        jk(&values, kb(session->isInitiator()));

        js(&keys,   ss((S) "persistsMessages"));        
        jk(&values, kb(session->getPersistMessages()));

        js(&keys,   ss((S) "sentLogin"));        
        jk(&values, kb(session->sentLogon()));

        js(&keys,   ss((S) "sentLogout"));        
        jk(&values, kb(session->sentLogout()));

        js(&keys,   ss((S) "isRunning"));        
        jk(&values, kb(session->isEnabled()));
 
        jk(&list, xD(keys, values));
    }

    return list;
}

////
// Dynamically links all of the functions in one go by returning a dictionary of symbols to dynamically
// loaded functions.
//
extern "C"
K load_library(K x)
{
    K keys = ktn(KS, 7);
    kS(keys)[0] = ss((S) "connect");
    kS(keys)[1] = ss((S) "listen");
    kS(keys)[2] = ss((S) "send");
    kS(keys)[3] = ss((S) "onrecv");
    kS(keys)[4] = ss((S) "setdefaults");
    kS(keys)[5] = ss((S) "getdefaults");
    kS(keys)[6] = ss((S) "getsessions");

    K values = ktn(0, 7);
    kK(values)[0] = dl((void *) fix_connect, 1);
    kK(values)[1] = dl((void *) fix_listen, 1);
    kK(values)[2] = dl((void *) fix_send, 1);
    kK(values)[3] = dl((void *) fix_onrecv, 1);
    kK(values)[4] = dl((void *) fix_set_defaults, 1);
    kK(values)[5] = dl((void *) fix_get_defaults, 1);
    kK(values)[6] = dl((void *) fix_get_sessions, 1);

    return xD(keys, values);
}
