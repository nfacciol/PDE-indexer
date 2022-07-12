#define _XOPEN_SOURCE 500
#include <ftw.h>
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
#include <time.h>
#include <thread>
#include <stdint.h>
#include <limits>
#include "cmdparse.h"

sqlite3 *db;
char* sql;
char *zErrMsg = 0;
int remoteConnection;
char* diskName;
char* databaseName;
int TOTAL_DISKS = 16;
struct flags{
        bool force;
        int max;
        bool match;
        std::string disk;
        std::string user;
};
const std::string WHITESPACE = " \n\r\t\f\v";

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


std::string diskNameHelper(int i){
    std::string disc = "sc_aepdeD";
    if(i < 10){
        disc += "0" + std::to_string(i) + ".db";
    }else{
        disc += std::to_string(i) + ".db";
    }
    return disc;
}

void print_file_result(sqlite3_stmt *&stmt, std::string &filename, int i, 
                        bool isLoop, std::string diskName, int max, bool match, std::string &username){
    std::string disc;
    if(isLoop)
        disc = diskNameHelper(i);
    else
        disc = diskName;
    databaseName = const_cast<char*>(disc.c_str());
    remoteConnection = sqlite3_open(databaseName, &db);
    std::string query;
    if(username == "") 
        query = "SELECT name from FILES WHERE name like \'%" + filename + "%\';";
    else{
        std::string a = "printf \'%s\' \"" + username + "\"| md5sum";
        const char* ha = const_cast<char*>(a.c_str());
        std::string hash = exec(ha);
        hash = trim(hash);
        int pos = hash.find("-");
        hash = hash.substr(0,pos);
        hash = trim(hash);
        query = "SELECT name from FILES WHERE name like \'%" + filename + "%\' AND id=\'" + hash + "\';";
    }
    const char* sqlSelect = const_cast<char*>(query.c_str());
    sqlite3_prepare_v2(db, sqlSelect, -1, &stmt, 0);
    int count = 0;
    while(sqlite3_step(stmt) != SQLITE_DONE){
        if(count == max){
            std::cout << max << " results displayed from query, do you wish to continue on this disk? (y/n)  ";
            char cont = std::cin.get();
            if(cont == 'n' ||  cont == 'N')
                return;
            else{
                count = 0;
                std::cout << "\n";
            }
        }
        const unsigned char* tmp_name = sqlite3_column_text(stmt,0);
        std::string tmp = (reinterpret_cast<const char*>(tmp_name));
        size_t found = tmp.find_last_of('/');
        std::string file = tmp.substr(found+1, tmp.size());
        if(!match){
            if(file.find(filename) != std::string::npos){
                std::cout << tmp_name << std::endl;
                count++;
            }
        }else{
            if(file == filename){
                std::cout << tmp_name << std::endl;
                count++;
            }
        }
    }
    sqlite3_reset(stmt);
    sqlite3_close(db);
}

void print_dir_result(sqlite3_stmt *&stmt, std::string &dirname, int i, 
                        bool isLoop, std::string diskName, int max, bool match, std::string &username){
    std::string disc;
    if(isLoop)
        disc = diskNameHelper(i);
    else
        disc = diskName;
    databaseName = const_cast<char*>(disc.c_str());
    remoteConnection = sqlite3_open(databaseName, &db);
    std::string query;
    if(username == "") 
        query = "SELECT name from DIRECTORIES WHERE name like \'%/" + dirname + "%\';";
    else{
        std::string a = "printf \'%s\' \"" + username + "\"| md5sum";
        const char* ha = const_cast<char*>(a.c_str());
        std::string hash = exec(ha);
        hash = trim(hash);
        int pos = hash.find("-");
        hash = hash.substr(0,pos);
        hash = trim(hash);
        query = "SELECT name from FILES WHERE name like \'%/" + dirname + "%\' AND id=\'" + hash + "\';";
    }
    const char* sqlSelect = query.c_str();
    sqlite3_prepare_v2(db, sqlSelect, -1, &stmt, 0);
    int count = 0;
    while(sqlite3_step(stmt) != SQLITE_DONE){
        if(count == max){
            std::cout << max << " results displayed from query, do you wish to continue on this disk? (y/n)  ";
            char cont = std::cin.get();
            if(cont == 'n' ||  cont == 'N')
                return;
            else{
                count = 0;
                std::cout << "\n";
            }
        }
        const unsigned char* tmp_name = sqlite3_column_text(stmt,0);
        std::string tmp = (reinterpret_cast<const char*>(tmp_name));
        size_t found = tmp.find_last_of('/');
        std::string file = tmp.substr(found+1, tmp.size());
        if(!match){
            if(file.find(dirname) != std::string::npos){
                std::cout << tmp_name << std::endl;
                count++;
            }
        }else{
            if(file == dirname){
                std::cout << tmp_name << std::endl;
                count++;
            }
        }
    }
    sqlite3_reset(stmt);
    sqlite3_close(db);
}

void find_file(std::string filename, bool force, int maximum, bool match, std::string disk, std::string user){
    sqlite3_stmt *stmt;
    if(disk == ""){
        if(force){
            int limit = std::numeric_limits<int>::max();
            for(int i = 1; i < TOTAL_DISKS + 1; i++){
                print_file_result(stmt, filename, i, true, "", limit, match, user);
            }
            sqlite3_finalize(stmt);
        }else{
            for(int i = 1; i < TOTAL_DISKS + 1; i++){
                print_file_result(stmt, filename, i, true, "", maximum, match, user);
            }
            sqlite3_finalize(stmt);
        }
    }else{
        if(force){
            int limit = std::numeric_limits<int>::max();
            print_file_result(stmt, filename, 0, false, disk, limit, match, user);
        }else{
            print_file_result(stmt, filename, 0, false, disk, maximum, match, user);
        }
    }
    return;
}

void find_dir(std::string dirname, bool force, int maximum, bool match, std::string disk, std::string user){
    sqlite3_stmt *stmt;
    if(disk == ""){
        if(force){
            int limit = std::numeric_limits<int>::max();
            for(int i = 1; i < TOTAL_DISKS + 1; i++){
                print_dir_result(stmt, dirname, i, true, "", limit, match, user);
            }
            sqlite3_finalize(stmt);
        }else{
            for(int i = 1; i < TOTAL_DISKS + 1; i++){
                print_dir_result(stmt, dirname, i, true, "", maximum, match, user);
            }
            sqlite3_finalize(stmt);
        }
    }else{
        if(force){
            int limit = std::numeric_limits<int>::max();
            print_dir_result(stmt, dirname, 0, false, disk, limit, match, user);
        }else{
            print_dir_result(stmt, dirname, 0, false, disk, maximum, match, user);
        }
    }
    return;
}

int setFlags(int argc, char** argv, flags &f, Parser *&p){
    if(p->findFlag("--force")){
            f.force = true;
    }
    if(p->findFlag("--max")){
        for(int i = 2; i < argc; i++){
            if(std::string(argv[i]) == "--max"){
                try{
                    std::string str(argv[i+1]);
                    f.max = stoi(str);
                }catch(...){
                    std::cout << "Error... please specify integer after max flag\n";
                    return 3;
                }
            }
        }
    }
    if(p->findFlag("--match")){
        f.match = true;
    }
    if(p->findFlag("--disk")){
        for(int i = 2; i < argc; i++){
            if(std::string(argv[i]) == "--disk"){
                try{
                    std::string str(argv[i+1]);
                    int num = std::stoi(str);
                    std::string diskNum = num < 10 ? "0" + std::to_string(num) : std::to_string(num);
                    std::string diskDb = "sc_aepdeD" + diskNum + ".db";
                    f.disk = diskDb;
                }catch(...){
                    std::cout << "Error... please specify disk number after disk flag\n";
                    return 3;
                }
            }
        }
    }
    if(p->findFlag("--user")){
        for(int i = 2; i < argc; i++){
            if(std::string(argv[i]) == "--user"){
                try{
                    std::string str(argv[i+1]);
                    f.user = str;
                }catch(...){
                    std::cout << "Error... please specify disk number after disk flag\n";
                    return 3;
                }
            }
        }
    }
    return 0;
}

int main(int argc, char** argv){
    Parser *p = new Parser(argc, argv);
    p->setFileName("./indexer.cpp");
    p->addArgument("-f", "Given a file name, query results that contain that file name\n\tand display its path on the disk (Note: use \'_\' to display all files)");
    p->addArgument("-d", "Given a directory name, query results that contain that directory name\n\tand display its path on the disk (Note: use \'_\' to display all directories)");
    p->addArgument("-r", "Given a regular expression, query results that mathc the regex\n\tand display its path on the disk");
    p->addFlag("--force", "If force is set then all query results will display without any pause");
    p->addFlag("--max", "If the max flag is given, specify the amount of results to display to the screen\n\tbefore a pause is shown");
    p->addFlag("--match", "If the match flag is given, results will only display names that exactly\n\tmatch the name of the given token");
    p->addFlag("--disk", "If the disk flag is given, results will be narrowed down to one\n\tdisk instead of all disks. (This can be useful to prevent hangs on large queries)\n\tPlease enter the disk number and do not manually enter a leading zero\n\ti.e \"--disk 2\" will search sc_aepdeD02.db, \"--disk 02\" will fail");
    p->addFlag("--user", "If the user flag is given, results will only be displayed for a username\n\tentered after the flag. (Note: if you want to search for files not owned\n\tby a particular user enter \'root_scaepdeD(DISKNUM)\' where DISKNUM is\n\tthe number of the disk you are searching (i.e 02)");

    if(argc < 2){
        p->printHelp();
        return 0;
    }

    
    flags f;
    f.force = false;
    f.match = false;
    f.max = 50;
    f.disk = "";
    f.user = "";
    int foobar = setFlags(argc, argv, f, p);

    if(foobar == 0){
        if(std::string(argv[1]) == "-f"){
            if(argc < 3){
                std::cout << "Error [-f]... please specify file name to search for, followed by flags\n";
                return 1;
            }
            std::string filename = argv[2];
            find_file(filename, f.force, f.max, f.match, f.disk, f.user);
        }else if(std::string(argv[1]) == "-d"){
            if(argc < 3){
                std::cout << "Error [-d]... please specify directory to search for, followed by flags\n";
                return 1;
            }
            std::string dirname = argv[2];
            find_dir(dirname, f.force, f.max, f.match, f.disk, f.user);
        }else if(std::string(argv[1]) == "-r"){
            if(argc < 3){
                std::cout << "Error [-r]... please specify regex to search for, followed by flags\n";
                return 1;
            }
        }else if(std::string(argv[1]) == "-h"){
            p->printHelp();
        }else{
            std::cout << "unrecognized command at position 1\n";
            p->printHelp();
        }
    }else{
        return 1;
    }

    delete p;
    return 0;
}