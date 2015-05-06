#ifndef KDB_CONVERT_H
#define KDB_CONVERT_H

#include "kx/k.h"

////
// Takes a formatting string and a time type and converts it into a string
// format that can be used in the FIX protocol. The times are taken to be 
// relative to UTC (i.e using the gmtime function) in order to match the
// behaviour of kdb+.
//
char *kdb_fmt_time(const char *str, const time_t time);

////
// Takes an atomic K object and converts it into a string that can be used
// in the FIX protocol.
//
char *kdb_str_atom(const K x);

/////
// Takes an atomic K collection (e.g. list/dictionary) and converts it int
// a string that can be used in the FIX protocol.
//
char *kdb_str_collection(const K x);

////
//
//
template<typename T>
char *kdb_read_argument(K dict, T key, const std::string& def);

#endif 
