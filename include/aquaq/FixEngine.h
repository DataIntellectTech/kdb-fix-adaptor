#ifndef FIX_ENGINE_H
#define FIX_ENGINE_H

#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"

#include "quickfix/fix40/NewOrderSingle.h"
#include "quickfix/fix41/NewOrderSingle.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix43/NewOrderSingle.h"
#include "quickfix/fix44/NewOrderSingle.h"
#include "quickfix/fix50/NewOrderSingle.h"

class FixEngineApplication : public FIX::Application
{
public:
	void onCreate(const FIX::SessionID& sessionID);
	void onLogon(const FIX::SessionID& sessionID);
	void onLogout(const FIX::SessionID& sessionID);
	void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID);
	void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
		throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon);
	void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw (FIX::DoNotSend);
	void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
		throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType);
};

#endif
