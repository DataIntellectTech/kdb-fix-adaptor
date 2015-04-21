#include "aquaq/FixEngine.h"

#include "kx/k.h"

using namespace FIX;

void FixEngineApplication::onCreate(const FIX::SessionID& sessionID)
{
	k(0, ".fix.create", ki(0), (K) 0);
}

void FixEngineApplication::onLogon(const FIX::SessionID& sessionID)
{
	k(0, ".fix.logon", ki(0), (K) 0);
}

void FixEngineApplication::onLogout(const FIX::SessionID& sessionID)
{
	k(0, ".fix.logout", ki(0), (K) 0);
}

void FixEngineApplication::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID)
{
	k(0, ".fix.toadmin", ki(0), (K) 0);
}

void FixEngineApplication::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
	throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon)
{
	k(0, ".fix.fromadmin", ki(0), (K) 0);
}

void FixEngineApplication::toApp(FIX::Message& message, const FIX::SessionID& sessionID)
	throw (FIX::DoNotSend)
{
	k(0, ".fix.toapp", ki(0), (K) 0);
}

void FixEngineApplication::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
	throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType)
{
	crack(message, sessionID);
	k(0, ".fix.fromapp", ki(0), (K) 0);
}

std::string FixEngineApplication::genOrderId() {
	std::stringstream stream;
	stream << ++_orderId;
	return stream.str();
}

std::string FixEngineApplication::genExecId() {
	std::stringstream stream;
	stream << ++_execId;
	return stream.str();
}

void FixEngineApplication::onMessage(const FIX40::NewOrderSingle& order, const FIX::SessionID& sessionID)
{
	k(0, ".fix.onmessage", ki(0), (K) 0);
}

void FixEngineApplication::onMessage(const FIX41::NewOrderSingle& order, const FIX::SessionID& sessionID)
{
	k(0, ".fix.onmessage", ki(0), (K) 0);
}

void FixEngineApplication::onMessage(const FIX42::NewOrderSingle& order, const FIX::SessionID& sessionID)
{
	k(0, ".fix.onmessage", ki(0), (K) 0);
}

void FixEngineApplication::onMessage(const FIX43::NewOrderSingle& order, const FIX::SessionID& sessionID)
{
	k(0, ".fix.onmessage", ki(0), (K) 0);
}

void FixEngineApplication::onMessage(const FIX44::NewOrderSingle& order, const FIX::SessionID& sessionID)
{
	k(0, ".fix.onmessage", ki(0), (K) 0);
}

void FixEngineApplication::onMessage(const FIX50::NewOrderSingle& order, const FIX::SessionID& sessionID)
{
	k(0, ".fix.onmessage", ki(0), (K) 0);
}
