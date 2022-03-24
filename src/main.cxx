#include <spdlog/spdlog.h>

#include <quickfix/Application.h>
#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/FieldMap.h>
#include <quickfix/ThreadedSocketAcceptor.h>
#include <quickfix/ThreadedSocketInitiator.h>
#include <quickfix/SessionSettings.h>

#include <pugixml.hpp>

#include "k.h"
#include "socketpair.h"

#include <config.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <algorithm>

#define unlikely(x) __builtin_expect((x),0)

static inline bool is_k_error(K x) { return xt == -128; }

std::unordered_map<int, std::string> typemap;

// socket pair for communication between fix engine instance and the main q thread.
//
// socket_pair[0] is for writing to the main thread.
// socket_pair[1] is for reading from the main thread.
int socket_pair[2];

class FixEngineApplication : public FIX::Application {
public:
    void onCreate(const FIX::SessionID &sessionID) override;

    void onLogon(const FIX::SessionID &sessionID) override;

    void onLogout(const FIX::SessionID &sessionID) override;

    void toAdmin(FIX::Message &message, const FIX::SessionID &sessionID) override;

    void fromAdmin(const FIX::Message &message,
                   const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;

    void toApp(FIX::Message &message, const FIX::SessionID &sessionID) throw(FIX::DoNotSend) override;

    void fromApp(const FIX::Message &message,
                 const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
};

K pu(I x) {
    return k(0, (S) "`timestamp$", kz(x / 8.64e4 - 10957), (K) nullptr);
}

//
// Converts a K object into its FIX string representation.
//
std::string kdb_type_to_fix_str(K x) {
    std::string result;

    if (-1 == x->t) {
        1 == x->g ? result.append("Y") : result.append("N");
    } else if (x->t <= -2 && x->t >= -11) {
        x = k(0, (S) "string ", r1(x), (K) nullptr);
    } else if (-12 == x->t) {
        long kdbtime = x->j;
        long unixtime = (kdbtime / 8.64e13 + 10957) * 8.64e4;
        time_t seconds(unixtime);
        tm *p = gmtime(&seconds);

        char buffer[80]; // make a power of 2
        strftime(buffer, 80, "%Y%m%d-%T.000", p);

        std::string timestr = std::to_string(kdbtime);

        buffer[18] = timestr[9];
        buffer[19] = timestr[10];
        buffer[20] = timestr[11];
        x = kp(buffer);
    } else if ((-14 == x->t) || (-13 == x->t)) {
        x = k(0, (S) R"({ssr[string x;".";""]})", r1(x), (K) nullptr);
    }

    return std::string{reinterpret_cast<S>(kC(x)), static_cast<size_t>(x->n)};
}

K parse_fix_to_timestamp(const std::string &field) {
    int year, month, day, hour, minute, second, ms;
    std::sscanf(field.c_str(), "%4d%2d%2d-%2d:%2d:%2d.%3d", &year, &month, &day, &hour, &minute, &second, &ms);

    std::tm tm{};
    tm.tm_year  = year - 1930;
    tm.tm_mon   = month - 1;
    tm.tm_mday  = day;
    tm.tm_hour  = hour;
    tm.tm_min   = minute;
    tm.tm_sec   = second;
    tm.tm_isdst = -1;

    auto time = std::chrono::high_resolution_clock::from_time_t(std::mktime(&tm));
    auto time_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(time).time_since_epoch().count();

    return ktj(-KP, time_ns + (ms * 1000000));
}

K parse_fix_to_date(const std::string &field) {
    int year, month, day;
    std::sscanf(field.c_str(), "%4d%2d%2d", &year, &month, &day);
    return kd(ymd(year, month, day));
}

K parse_fix_to_time(const std::string &field) {
    int hour, minute, second;
    std::sscanf(field.c_str(), "%2d:%2d:%2d", &hour, &minute, &second);
    return kt(((hour * 3600) + (minute * 60) + second) * 1000);
}

K parse_fix_to_kobj(const std::string &field, const std::string &type) {
    if ("FLOAT" == type) {
        return kf(std::stof(field));
    } else if ("STRING" == type) {
        return kp(const_cast<char *>(field.c_str()));
    } else if ("INT" == type) {
        return ki(std::stoi(field));
    } else if ("CHAR" == type) {
        return kc(field[0]);
    } else if ("BOOLEAN" == type) {
        return kb("Y" == field);
    } else if ("TIMESTAMP" == type) {
        return parse_fix_to_timestamp(field);
    } else if ("DATE" == type) {
        return parse_fix_to_date(field);
    } else if ("TIME" == type) {
        return parse_fix_to_time(field);
    } else {
        return kp(const_cast<char *>(field.c_str()));
    }
}

static void
fill_from_iterators(FIX::FieldMap::Fields::const_iterator begin, FIX::FieldMap::Fields::const_iterator end, K *keys,
                    K *values) {
    for (auto it = begin; it != end; it++) {
        J tag = (J) it->getTag();
        auto str = it->getString().c_str();
        auto found = typemap.find(tag);
        ja(keys, &tag);

        if (55 == tag) {
            jk(values, ks(const_cast<char *>(str)));
        } else {
            jk(values, parse_fix_to_kobj(str, found->second));
        }
    }
}

static K convert_to_dictionary(const FIX::Message &message) {
    K keys = ktn(KJ, 0);
    K values = ktn(0, 0);

    auto header = message.getHeader();
    auto trailer = message.getTrailer();

    fill_from_iterators(std::begin(header), std::end(header), &keys, &values);
    fill_from_iterators(std::begin(message), std::end(message), &keys, &values);
    fill_from_iterators(std::begin(trailer), std::end(trailer), &keys, &values);

    return xD(keys, values);
}

static inline void send_bytes(ssize_t expected_bytes, G *buffer) {
    ssize_t bytes_sent = 0;
    while (bytes_sent < expected_bytes > 0) {
        ssize_t written = send(socket_pair[0], &buffer[bytes_sent], (int) (expected_bytes - bytes_sent), 0);
        if (unlikely(written == -1)) {
            spdlog::warn("no bytes written to socket pair: {}", std::strerror(errno));
            continue;
        }
        if (unlikely(written == 0)) {
            spdlog::error("socket closed while expecting bytes");
            return; // todo :: return an error code to allow this to be handled properly?
        }
        bytes_sent += written;
    }
}

static void send_data(K x) {
    K serialized = ee(b9(-1, x));
    r0(x);

    if (unlikely(is_k_error(serialized))) {
        spdlog::error("problem serializing fix message: {}", serialized->s);
        r0(serialized);
        return;
    }

    // write the size of the serialized K object followed by the K object serialized...
    send_bytes(sizeof(serialized->n), reinterpret_cast<G *>(&serialized->n));
    send_bytes(serialized->n, kG(serialized));

    r0(serialized);
}

void FixEngineApplication::onCreate(const FIX::SessionID &sessionID) {}

void FixEngineApplication::onLogon(const FIX::SessionID &sessionID) {}

void FixEngineApplication::onLogout(const FIX::SessionID &sessionID) {}

void FixEngineApplication::toAdmin(FIX::Message &message, const FIX::SessionID &sessionID) {}

void FixEngineApplication::toApp(FIX::Message &message, const FIX::SessionID &sessionID) throw(FIX::DoNotSend) {}

void FixEngineApplication::fromAdmin(const FIX::Message &message,
                                     const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) {
    send_data(convert_to_dictionary(message));
}

void FixEngineApplication::fromApp(const FIX::Message &message,
                                   const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
    send_data(convert_to_dictionary(message));
}

extern "C"
K send_message_dict(K x) {
    if (x->t != 99 || kK(x)[0]->t != 7 || kK(x)[1]->t != 0) {
        return krr((S) "type");
    }

    K keys = kK(x)[0];
    K values = kK(x)[1];

    FIX::Message message;
    FIX::Header &header = message.getHeader();

    for (int i = 0; i < keys->n; i++) {
        auto rep = kdb_type_to_fix_str(kK(values)[i]);
        int tag = kJ(keys)[i];
        if (tag == 35 || tag == 8 || tag == 49 || tag == 56) {
            header.setField(tag, rep);
        } else {
            message.setField(tag, rep);
        }
    }

    try {
        FIX::Session::sendToTarget(message);
    } catch (FIX::SessionNotFound &ex) {
        spdlog::error("unable to send fix message - session not found");
    }

    return (K) nullptr;
}

static inline void read_bytes(ssize_t expected_bytes, G *buffer) {
    ssize_t bytes_read = 0;
    while (bytes_read < expected_bytes) {
        ssize_t read = recv(socket_pair[1], &buffer[bytes_read], (int) (expected_bytes - bytes_read), 0);
        if (unlikely(read == -1)) {
            spdlog::warn("no bytes read from socket pair: {}", std::strerror(errno));
            continue;
        }
        if (unlikely(read == 0)) {
            spdlog::error("socket closed while expecting bytes");
            return; // todo :: return an error code to allow this to be handled properly?
        }
        bytes_read += read;
    }
}

extern "C"
K receive_data(I x) {
    J size = 0;
    read_bytes(sizeof(size), reinterpret_cast<G *>(&size));

    K bytes = ktn(KG, size);
    read_bytes(size, kG(bytes));

    K deserialized = ee(d9(bytes));
    r0(bytes);

    if (unlikely(is_k_error(deserialized))) {
        spdlog::error("deserializing fix message: {}", (deserialized->s ? deserialized->s : ""));
        r0(deserialized);
    } else {
        K result = ee(k(0, (S) ".fix.onrecv", deserialized, (K) nullptr));

        if (unlikely(is_k_error(result))) {
            spdlog::error("problem sending fix message via .fix.onrecv: {}", (result->s ? result->s : ""));
        }

        r0(result);
    }

    return (K) nullptr;
}

template<typename T>
K create_threaded_socket(K x) {
    if (unlikely(x->t != -11)) {
        return krr((S) "type");
    }

    std::string settingsPath = std::string(x->s);
    settingsPath.erase(std::remove(std::begin(settingsPath), std::end(settingsPath), ':'), std::end(settingsPath));

    auto settings = new FIX::SessionSettings(settingsPath);
    auto application = new FixEngineApplication;
    auto store = new FIX::FileStoreFactory(*settings);
    auto log = new FIX::FileLogFactory(*settings);

    dumb_socketpair(socket_pair, 0);
    sd1(socket_pair[1], receive_data);

    T *socket = new T(*application, *store, *settings, *log);
    socket->start();

    return (K) nullptr;
}

extern "C"
K create_initiator(K x) { return create_threaded_socket<FIX::ThreadedSocketInitiator>(x); }

extern "C"
K create_acceptor(K x) { return create_threaded_socket<FIX::ThreadedSocketAcceptor>(x); }

extern "C"
K create(K x, K y) {
    if (unlikely(-11 != x->t)) {
        return krr((S) "type");
    }

    K z = ks((S) "sessions/sample.ini");

    if (-11 == y->t) {
        r0(z);
        z = ks((S) y->s);
    } else {
        std::cout << "Defaulting to sample.ini config" << std::endl;
    }

    if (strcmp("initiator", x->s) == 0) {
        K ret = create_threaded_socket<FIX::ThreadedSocketInitiator>(z);
        r0(z);
        return ret;
    } else if (strcmp("acceptor", x->s) == 0) {
        K ret = create_threaded_socket<FIX::ThreadedSocketAcceptor>(z);
        r0(z);
        return ret;
    } else {
        return krr((S) "type");
    }
}

// We have an empty implementation of on_recv for the case where the q code doesn't provide
// one for us.
extern "C" K on_recv(K x) { return (K) nullptr; }

extern "C"
K get_version_info(K x) {
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

std::unordered_map<std::string, std::string> &get_conversion_map() {
    // Mapping of FIX types to KDB types used when decoding FIX messages. If a type is not
    // present in this map then it will be converted into a string.
    static std::unordered_map<std::string, std::string> STATIC_CONVERSION_MAP = {
            {"STRING",              "STRING"},
            {"MULTIPLEVALUESTRING", "STRING"},
            {"PRICE",               "FLOAT"},
            {"CHAR",                "CHAR"},
            {"INT",                 "INT"},
            {"AMT",                 "FLOAT"},
            {"CURRENCY",            "FLOAT"},
            {"QTY",                 "FLOAT"},
            {"EXCHANGE",            "SYM"},
            {"UTCTIMESTAMP",        "TIMESTAMP"},
            {"BOOLEAN",             "BOOLEAN"},
            {"LOCALMKTDATE",        "DATE"},
            {"DATA",                "STRING"},
            {"LENGTH",              "FLOAT"},
            {"FLOAT",               "FLOAT"},
            {"PRICEOFFSET",         "FLOAT"},
            {"MONTHYEAR",           "STRING"},
            {"DAYOFMONTH",          "STRING"},
            {"UTCDATE",             "DATE"},
            {"UTCTIMEONLY",         "TIME"},
            {"COUNTRY",             "STRING"},
            {"DATE",                "STRING"},
            {"EXCHANGE",            "SYM"},
            {"LANGUAGE",            "STRING"},
            {"MULTIPLECHARVALUE",   "STRING"},
            {"MULTIPLESTRINGVALUE", "STRING"},
            {"NUMINGROUP",          "INT"},
            {"PERCENTAGE",          "FLOAT"},
            {"PRICEOFFSET",         "FLOAT"},
            {"SEQNUM",              "INT"},
            {"TIME",                "STRING"},
            {"UTCDATE",             "DATE"},
            {"UTCTIMEONLY",         "STRING"},
    };

    // Initialized only on first call to function due to static storage.
    return STATIC_CONVERSION_MAP;
}

std::string fix_type_to_kdb_type(const std::string &s) {
    auto conversion_map = get_conversion_map();
    auto found = conversion_map.find(s);
    return found != std::end(conversion_map) ? found->second : "STRING";
}

void initialize_type_map() {
    pugi::xml_document doc;
    if (!doc.load_file("./spec/FIX42.xml")) throw std::runtime_error("XML could not be loaded");
    pugi::xml_node fields = doc.child("fix").child("fields");

    for (auto field = fields.child("field"); field; field = field.next_sibling("field")) {
        int value = field.attribute("number").as_int();
        std::string fieldattr = fix_type_to_kdb_type(field.attribute("type").value());
        typemap.insert({value, fieldattr});
    }
}

extern "C"
K LoadLibrary(K x) {
    spdlog::info(" release » {}",  BUILD_PROJECT_VERSION);
    spdlog::info(" quickfix » {}", BUILD_QUICKFIX_VERSION);
    spdlog::info(" pugixml » {}", BUILD_PUGIXML_VERSION);
    spdlog::info(" os » {} ", BUILD_OPERATING_SYSTEM);
    spdlog::info(" arch » {}", BUILD_PROCESSOR);
    spdlog::info(" build datetime » {}", BUILD_DATE);
    spdlog::info(" kdb compatibility » {}", BUILD_KX_VER);
    spdlog::info(" compiler flags » {}", BUILD_COMPILER_FLAGS);

    K keys = ktn(KS, 6);
    K values = ktn(0, 6);

    kS(keys)[0] = ss((S) "initiator");
    kS(keys)[1] = ss((S) "acceptor");
    kS(keys)[2] = ss((S) "send");
    kS(keys)[3] = ss((S) "onrecv");
    kS(keys)[4] = ss((S) "create");
    kS(keys)[5] = ss((S) "version");

    kK(values)[0] = dl((void *) create_initiator, 1);
    kK(values)[1] = dl((void *) create_acceptor, 1);
    kK(values)[2] = dl((void *) send_message_dict, 1);
    kK(values)[3] = dl((void *) on_recv, 1);
    kK(values)[4] = dl((void *) create, 2);
    kK(values)[5] = dl((void *) get_version_info, 1);

    // todo :: this should be done lazily when a session is created based on the acceptor or initiator config.
    // todo :: should keep different maps for different sessions as each could be using different message formats.
    initialize_type_map();

    return xD(keys, values);
}