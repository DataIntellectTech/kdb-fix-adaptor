#include <memory>
#include <string>
#include <sstream>
#include <iostream>

#include "kx/k.h"

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

#include "aquaq/FixEngine.h"

class FixSessionSettings
{
public:
    std::string BeginString = "FIX.4.4";
    std::string SourceCompID = "BuySideID";
    std::string TargetCompID = "SellSideID";
    std::string DataDictionary = "config/spec/FIX.4.4.xml";
    std::string AppDataDictionary = "config/spec/FIX.4.4.xml";
    std::string PersistMessages = "Y";
    std::string FileStorePath = "store";
    std::string FileLogPath = "log";
    std::string ReconnectInterval = "30";
    std::string HeartBtInt = "15";
    std::string StartTime = "00:00:00";
    std::string EndTime = "23:59:59";
    std::string ConnectionType = "acceptor";
    std::string SocketPort = "7070";
    std::string SocketHost = "127.0.0.1";
    std::string SocketReuseAddress = "Y";
    std::string SessionQualifier = "0";
            
    friend std::ostream& operator<<(std::ostream& output, const FixSessionSettings& settings);
};

std::ostream& operator<<(std::ostream& output, const FixSessionSettings& settings)
{
    output << "[DEFAULT]" << std::endl;
    output << "BeginString=" << settings.BeginString << std::endl;
    output << "SenderCompID=" << settings.SourceCompID << std::endl;
    output << "TargetCompID=" << settings.TargetCompID << std::endl;
    output << "DataDictionary=" << settings.DataDictionary << std::endl;
    output << "AppDataDictionary=" << settings.AppDataDictionary << std::endl;
    output << "PersistMessages=" << settings.PersistMessages << std::endl;
    output << "FileStorePath=" << settings.FileStorePath << std::endl;
    output << "FileLogPath=" << settings.FileLogPath << std::endl;
    output << "ReconnectInterval=" << settings.ReconnectInterval << std::endl;
    output << "HeartBtInt=" << settings.HeartBtInt << std::endl;
    output << "StartTime=" << settings.StartTime << std::endl;
    output << "EndTime=" << settings.EndTime << std::endl;
    output << "ConnectionType=" << settings.ConnectionType << std::endl;
 
    output << "[SESSION]" << std::endl;

    if (settings.ConnectionType == "acceptor") {
        output << "SocketAcceptPort=" << settings.SocketPort << std::endl;
        output << "SocketReuseAddress=" << settings.SocketReuseAddress << std::endl;
    } else {
        output << "SocketConnectPort=" << settings.SocketPort << std::endl;
        output << "SocketConnectHost=" << settings.SocketHost << std::endl;
        output << "SessionQualifier=" << settings.SessionQualifier << std::endl;
    }

    return output;
}

////
// Object to manage the configuration & lifetime of a single side of a FIX
// session (acceptor or initiator) and handle configuration/startup.
//
template<typename T>
class FixSession
{
public:
    class Builder;

    FixSession(const FixSessionSettings& settings);
    ~FixSession();

    ////
    // Starts the acceptor/initiator and allocates any required resources the first time
    // that it is run.
    //
    FixSession& start(void);
    FixSession& stop(void);

    FixSession& sendMessage(const K& dict);

    ////
    // Returns the SessionID that is associated with the session. This can be used to
    // send messages and lookup connections. 
    //
    FIX::SessionID* id(void);
private:
    std::shared_ptr<FIX::SessionID> mSessionId;
    std::shared_ptr<FIX::SessionSettings> mSettings;
    std::shared_ptr<FixEngineApplication> mApplication;
    std::shared_ptr<FIX::FileStoreFactory> mStore;
    std::shared_ptr<FIX::ScreenLogFactory> mLog;
    std::shared_ptr<T> mThread;
};

template<typename T>
FixSession<T>& FixSession<T>::start(void) { mThread->start(); return *this; }

template<typename T>
FixSession<T>& FixSession<T>::stop(void) { mThread->stop(); return *this; }

template<typename T>
FixSession<T>& FixSession<T>::sendMessage(const K& dict) { std::cout << "sending a message from kdb+ to fix engine" << std::endl; return *this; }

template<typename T>
FIX::SessionID* FixSession<T>::id(void) { return mSessionId.get(); }

template<typename T>
FixSession<T>::FixSession(const FixSessionSettings& settings)
{
    // TODO :: Make the default config file location available from the settings?
    mSessionId = std::make_shared<FIX::SessionID>(settings.BeginString, settings.SourceCompID, settings.TargetCompID);
    mSettings = std::make_shared<FIX::SessionSettings>("config/acceptor-config.ini");

    std::stringstream stream;
    stream << settings;
    stream >> *mSettings;

    mApplication = std::make_shared<FixEngineApplication>();
    mStore = std::make_shared<FIX::FileStoreFactory>(*mSettings);
    mLog = std::make_shared<FIX::ScreenLogFactory>(*mSettings);

    mThread = std::make_shared<T>(*mApplication, *mStore, *mSettings, *mLog);
}

////
// Builds a new FixSession object and validates the arguments that will be passed to the object
// when it is constructed.
//
template<typename T>
class FixSession<T>::Builder
{
public:
    Builder() { } // TODO :: force required arguments in the constructor

    ////
    // Sets the version of the FIX session. Should be one of the supported FIX versions as listed below. Passing
    // a fix version that cannot be found in the configuration folder will cause a runtime exception to be thrown.
    //
    // Supported Versions:
    //      FIX.4.0
    //      FIX.4.1
    //      FIX.4.2
    //      FIX.4.3
    //      FIX.4.4
    //      FIX.5.0
    //      FIX.5.0SP1
    //      FIX.5.0SP2
    //      FIXT1.1
    //
    // Additional fix specifications can be added to the config/spec folder.
    //
    Builder& setVersion(const std::string& version) { this->settings.BeginString = version; return *this; }

    ////
    // Sets the target id of the FIX session. 
    //
    Builder& setTargetId(const std::string& targetId) { this->settings.TargetCompID = targetId; return *this; }

    ////
    // Sets the source id of the FIX session.
    //
    Builder& setSourceId(const std::string& sourceId) { this->settings.SourceCompID = sourceId; return *this; }
    
    ////
    // Sets the session qualifier to be used for the FIX session. This is only valid for initiators and will be ignored
    // on acceptors. Should be a unique string to identify the session, can be left empty if you plan to have just one
    // initiator on the client side.
    //
    Builder& setQualifier(const std::string& qualifier) { this->settings.SessionQualifier = qualifier; return *this; }
    
    ////
    // Sets the port to be used by the FIX session. For initiators, this is used to indicate which port they should attempt
    // to connect on the target FIX engine.
    //
    // TODO :: Add the SocketAddress setting instead of defaulting to localhost
    //
    Builder& setPort(const std::string& port) { this->settings.SocketPort = port; return *this; }
    
    ////
    // Sets the host to be used by the initiator process when opening a connection to the FIX engine.
    // 
    //
    Builder& setHost(const std::string& host) { this->settings.SocketHost = host; return *this; }
    
    ////
    //
    //
    Builder& setType(const std::string& type) { this->settings.ConnectionType = type; return *this; }

    ////
    //
    FixSession<T>* build() { return new FixSession<T>(this->settings); }
private:
    FixSessionSettings settings;
};

