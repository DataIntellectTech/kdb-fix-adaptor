#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <algorithm>

#include <quickfix/Application.h>
#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/FieldMap.h>
#include <quickfix/ThreadedSocketAcceptor.h>
#include <quickfix/ThreadedSocketInitiator.h>
#include <quickfix/SessionSettings.h>
#include <pugixml.hpp>
#include <kx/k.h>
#include "socketpair.h"

#include <config.h>
#include <cstring>

std::unordered_map<int, std::string> typemap;

int sockets[2];

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

struct Tm : std::tm {
    Tm(const int year, const int month, const int mday, const int hour,
       const int min, const int sec, const int isDST = -1)
            : tm() {
        tm_year = year - 1900;
        tm_mon = month - 1;
        tm_mday = mday;
        tm_hour = hour;
        tm_min = min;
        tm_sec = sec;
        tm_isdst = isDST;
    }

    template<typename Clock_t = std::chrono::high_resolution_clock>
    auto to_time_point() -> typename Clock_t::time_point {
        auto time_c = mktime(this);
        return Clock_t::from_time_t(time_c);
    }
};

K pu(I x) {
    return k(0, (S) "`timestamp$", kz(x / 8.64e4 - 10957), (K) nullptr);
}

//
// Converts a K object into
//
std::string kdb_type_to_fix_str(K x) {
    if (-1 == x->t) {
        if (1 == x->g) {
            x = kp(const_cast<char *>("Y"));
        } else {
            x = kp(const_cast<char *>("N"));
        }
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
    } else if (-14 == x->t) {
        x = k(0, (S) R"({ssr[string x;".";""]})", x, (K) nullptr);
    }

    std::string rep;

    for (int i = 0; i < x->n; i++) {
        rep += kC(x)[i];
    }

    return rep;
}

K strtotemporal(const std::string &datestring) {
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

    auto tp_micro = Tm(year, month, day, hour, minute, second).to_time_point();
    K tp = pu(std::chrono::high_resolution_clock::to_time_t(tp_micro)); // ms from Unix epoch
    auto time = tp->j + ms * 1e6;

    return kj(time);
}

int fix_field_to_date(const std::string &date) {
    int year, month, day;
    std::sscanf(date.c_str(), "%4d%2d%2d", &year, &month,
                &day); // todo :: handle read errors with sscanf - need to check number if variable read matches
    struct std::tm a = {0, 0, 0, day, month - 1, year - 1900};
    struct std::tm b = {0, 0, 0, 1, 0, 100};
    std::time_t x = std::mktime(&a);
    std::time_t y = std::mktime(&b);
    return std::difftime(x, y) / (60 * 60 * 24);
}

int fix_field_to_time(const std::string &time) {
    int hour, minute, second;
    sscanf(time.c_str(), "%2d:%2d:%2d", &hour, &minute, &second);
    struct std::tm a = {second, minute, hour, 1, 0, 0};
    struct std::tm b = {0, 0, 0, 1, 0, 0};
    std::time_t x = std::mktime(&a);
    std::time_t y = std::mktime(&b);
    int difference = std::difftime(x, y) * 1e3;
    return difference;
}

K convertmsgtype(std::string field, const std::string &type) {
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
        auto ts = strtotemporal(field);
        return ktj(-KP, ts->j);
    } else if ("DATE" == type) {
        return kd(fix_field_to_date(field));
    } else if ("TIME" == type) {
        return kt(fix_field_to_time(field));
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
            jk(values, convertmsgtype(str, found->second));
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

static void write_to_socket(K x) {
    static char buffer[8192];

    K bytes = b9(-1, x);
    r0(x);

    memcpy(&buffer, (char *) &bytes->n, sizeof(J));
    memcpy(&buffer[sizeof(J)], kG(bytes), (size_t) bytes->n);

    send(sockets[0], buffer, (int) sizeof(J) + bytes->n, 0);
    r0(bytes);
}

void FixEngineApplication::onCreate(const FIX::SessionID &sessionID) {}

void FixEngineApplication::onLogon(const FIX::SessionID &sessionID) {}

void FixEngineApplication::onLogout(const FIX::SessionID &sessionID) {}

void FixEngineApplication::toAdmin(FIX::Message &message, const FIX::SessionID &sessionID) {}

void FixEngineApplication::toApp(FIX::Message &message, const FIX::SessionID &sessionID) throw(FIX::DoNotSend) {}

void FixEngineApplication::fromAdmin(const FIX::Message &message,
                                     const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) {
    write_to_socket(convert_to_dictionary(message));
}

void FixEngineApplication::fromApp(const FIX::Message &message,
                                   const FIX::SessionID &sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
    write_to_socket(convert_to_dictionary(message));
}

extern "C"
K send_message_dict(K x) {
    if (x->t != 99 || kK(x)[0]->t != 7 || kK(x)[1]->t != 0) {
        return krr((S) "type");
    }

    K keys = kK(x)[0];
    K values = kK(x)[1];

    std::string beginString;
    std::string senderCompId;
    std::string targetCompId;
    std::string sessionQualifier;

    FIX::Message message;
    FIX::Header &header = message.getHeader();

    for (int i = 0; i < keys->n; i++) {
        int tag = kJ(keys)[i];

        auto rep = kdb_type_to_fix_str(kK(values)[i]);

        if (tag == 35 || tag == 8 || tag == 49 || tag == 56) {
            header.setField(tag, rep);
        } else {
            message.setField(tag, rep);
        }
    }

    try {
        FIX::Session::sendToTarget(message);
    } catch (FIX::SessionNotFound &ex) {
        std::cout << "unable to send message - session not found" << std::endl;
    }

    return (K) nullptr;
}

static inline void read_bytes(ssize_t expected_bytes, char (*buf)[4096]) {
    ssize_t bytes_read = 0;
    do { bytes_read += recv(sockets[1], &buf[bytes_read], (int) (expected_bytes - bytes_read), 0); }
    while (bytes_read < expected_bytes);
}

extern "C"
K receive_data(I x) {
    static char buf[4096];
    J size = 0;

    read_bytes(sizeof(J), &buf);
    memcpy(&size, buf, sizeof(J));

    K bytes = ktn(KG, size);

    read_bytes(size, &buf);
    memcpy(kG(bytes), &buf, (size_t) size);
    K r = k(0, (char *) ".fix.onrecv", d9(bytes), (K) nullptr);
    r0(bytes);

    if (r != nullptr) { r0(r); }

    return (K) nullptr;
}

template<typename T>
K create_threaded_socket(K x) {
    if (x->t != -11) {
        return krr((S) "type");
    }

    std::string settingsPath = std::string(x->s);
    settingsPath.erase(std::remove(std::begin(settingsPath), std::end(settingsPath), ':'), std::end(settingsPath));

    auto settings = new FIX::SessionSettings(settingsPath);
    auto application = new FixEngineApplication;
    auto store = new FIX::FileStoreFactory(*settings);
    auto log = new FIX::FileLogFactory(*settings);

    dumb_socketpair(sockets, 0);
    sd1(sockets[1], receive_data);

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
    if (-11 != x->t) {
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

    for (pugi::xml_node field = fields.child("field"); field; field = field.next_sibling("field")) {
        int value = field.attribute("number").as_int();
        std::string fieldattr = fix_type_to_kdb_type(field.attribute("type").value());
        typemap.insert({value, fieldattr});
    }
}

extern "C"
K LoadLibrary(K x) {
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
    printf("████████████████████████████████████████████████████\n");

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
    initialize_type_map();

    return xD(keys, values);
}