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

class FixEngineApplication : public FIX::Application, public FIX::MessageCracker 
{
public:
	FixEngineApplication() : _orderId(0), _execId(0) {}

	void onCreate(const FIX::SessionID& sessionID);
	void onLogon(const FIX::SessionID& sessionID);
	void onLogout(const FIX::SessionID& sessionID);
	void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID);
	void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
		throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon);
	void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw (FIX::DoNotSend);
	void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
		throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType);

	void onMessage(const FIX40::NewOrderSingle& order, const FIX::SessionID& sessionID);
	void onMessage(const FIX41::NewOrderSingle& order, const FIX::SessionID& sessionID);
	void onMessage(const FIX42::NewOrderSingle& order, const FIX::SessionID& sessionID);
	void onMessage(const FIX43::NewOrderSingle& order, const FIX::SessionID& sessionID);
	void onMessage(const FIX44::NewOrderSingle& order, const FIX::SessionID& sessionID);
	void onMessage(const FIX50::NewOrderSingle& order, const FIX::SessionID& sessionID);

	std::string genOrderId();
	std::string genExecId();
private:
	int _orderId;
	int _execId;
};

#endif
