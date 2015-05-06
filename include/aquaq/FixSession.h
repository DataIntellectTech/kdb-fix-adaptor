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
    std::string DefaultApplVerID = "";
    std::string UseDataDictionary = "";
    std::string SendResetSeqNumFlag = "";
    std::string SendRedundantResendRequests = "";
    std::string CheckCompID = "";
    std::string CheckLatency = "";
    std::string MaxLatency = "";
    std::string SocketNoDelay = "";
    std::string SendBufferSize = "";
    std::string RecieveBufferSize = "";
    std::string ValidateLengthAndChecksum = "";
    std::string ValidateFieldsOutOfOrder = "";
    std::string ValidateFieldsHaveValues = "";
    std::string ValidateUserDefinedFields = "";
    std::string LogonTimeout = "";
    std::string LogoutTimeout = "";
    std::string ResetOnLogon = "";
    std::string ResetOnLogout = "";
    std::string ResetOnDisconnect = "";
    std::string HttpAcceptPort = "";

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
    if (settings.ConnectionType == "acceptor") {
        mSessionId = std::make_shared<FIX::SessionID>(settings.BeginString, settings.SourceCompID, settings.TargetCompID);
    } else {
        mSessionId = std::make_shared<FIX::SessionID>(settings.BeginString, settings.SourceCompID, settings.TargetCompID, settings.SessionQualifier);
    }
    mSettings = std::make_shared<FIX::SessionSettings>("config/acceptor-config.ini");
    
    std::cout << "[FixSession<T>::FixSession] before settings: " << std::endl << *mSettings << std::endl;

    const FIX::Dictionary& dict = mSettings->get();
    std::string beginString = dict.getString("BeginString", true);
    std::cout << " [-] The begin string is: " << std::endl;
 
    // Create a new dictionary to hold our settings.
    FIX::Dictionary newSettings;
    newSettings.setString("Version",        "FIX.4.4");
    newSettings.setString("TargetCompID",   "BuySideID");
    newSettings.setString("SourceCompID",   "SellSideID");
    newSettings.setString("Port",           "7025");
    newSettings.setString("Host",           "127.0.0.1");
    newSettings.setString("ConnectionType", "acceptor");

    // Use the SessionID and the settings.set(sessionID, dictionary) to initialize the other sessions?

    // Merging the old dictionary into the  new one will fill in any fields
    // that are missing from the new dictionary.
    newSettings.merge(dict);
  
    // Set the new dictionary in the settings object. 
    mSettings->set(newSettings);

    std::stringstream defaults;
    defaults << *mSettings;
    std::cout << " [+] loading the default settings from the dictionary/config " << std::endl << defaults.str() << std::endl; 
 
    std::stringstream stream;
    stream << defaults.str();
 
    std::cout << " [+] writing the default settings to the dictionary/config " << std::endl << stream.str() << std::endl;
    stream >> *mSettings;

    std::cout << " [+] finalized in-memory settings" << std::endl <<  *mSettings << std::endl;

    mApplication = std::make_shared<FixEngineApplication>();
    mStore = std::make_shared<FIX::FileStoreFactory>(*mSettings);
    mLog = std::make_shared<FIX::ScreenLogFactory>(*mSettings);

    mThread = std::make_shared<T>(*mApplication, *mStore, *mSettings, *mLog);
}

