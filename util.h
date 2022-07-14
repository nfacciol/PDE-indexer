/***************************************
*utility header file for helper functions
****************************************/
#include <iostream>
#include <sqlite3.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <stdlib.h>

const std::string WHITESPACE = " \n\r\t\f\v";
char *zErrMsg = 0;

static int callback(void *NotUsed, int argc, char **argv, char **azColName);
void wasSQLOperationSuccessful(int &rc, std::string operation);
std::string ltrim(const std::string &s);
std::string rtrim(const std::string &s);
std::string trim(const std::string &s);
std::string exec(const char* cmd);


/***************************************************************************
 * SQLITE functions
 * ************************************************************************/
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

void wasSQLOperationSuccessful(int &rc, std::string operation){
    if (rc != SQLITE_OK){
        std::cout << operation << std::endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } //else {
    //     std::cout << operation + " completed successfully\n";
    // }
}

/********************************
 * helper functions
*********************************/ 
std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
 
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
 
std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

std::string exec(const char* cmd){
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}