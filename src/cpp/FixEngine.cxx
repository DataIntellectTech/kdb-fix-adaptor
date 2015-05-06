#include "aquaq/FixEngine.h"

#include "kx/k.h"

#include <typeinfo>
#include <algorithm>
#include <functional>

using namespace FIX;

void FixEngineApplication::onCreate(const FIX::SessionID& sessionID)
{
	k(0, (S) ".fix.create", ki(0), (K) 0);
}

void FixEngineApplication::onLogon(const FIX::SessionID& sessionID)
{
	k(0, (S) ".fix.logon", ki(0), (K) 0);
}

void FixEngineApplication::onLogout(const FIX::SessionID& sessionID)
{
	k(0, (S) ".fix.logout", ki(0), (K) 0);
}

void FixEngineApplication::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID)
{
	k(0, (S) ".fix.toadmin", ki(0), (K) 0);
}

void FixEngineApplication::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
	throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon)
{
	k(0, (S) ".fix.fromadmin", ki(0), (K) 0);
}

void FixEngineApplication::toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw (FIX::DoNotSend) {
	k(0, (S) ".fix.toapp", ki(0), (K) 0);
}

static K FillFromIterators(FieldMap::Fields::const_iterator begin, FieldMap::Fields::const_iterator end, K* keys, K* values)
{
    for (auto it = begin; it != end; it++) {
        auto tag = it->first;
        ja(keys, &tag);
        auto str = it->second.getString().c_str();
        js(values, (S) str);
    }
}

static K ConvertToDictionary(const FIX::Message& message)
{
    K keys = ktn(KI, 0);
    K values = ktn(KS, 0);

    auto header = message.getHeader();
    auto trailer = message.getTrailer();

    FillFromIterators(header.begin(), header.end(), &keys, &values);
    FillFromIterators(message.begin(), message.end(), &keys, &values);
    FillFromIterators(trailer.begin(), trailer.end(), &keys, &values);

    std::cout << "created dictionary with " << keys->n << " keys and " << values->n << " values." << std::endl;
    return xD(keys, values);
}

void FixEngineApplication::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw (FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
	k(0, (S) ".fix.onrecv", ConvertToDictionary(message), (K) 0);
}


