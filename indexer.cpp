/******************************************************************************
 * Author: Nicholas Facciola
 * Indexer cpp is the main search file for finding files and directories
 * across PDE disks on the santa clara server.
 * 
 * Basic supported functions are find file, find dir, and regex. The search
 * will cover every disk unless otherwise noted by the user
 * 
 * FIND FILE
 * find file works by pulling each path from the database which contains the 
 * input given by the user. To prevent all paths with the keywords being shown
 * find file finds the index of the last slash and checks to see if the filename
 * given is still contained within the last part of the text. If this is true
 * then the full path is displayed to the user.
 * 
 * FIND DIR
 * find dir works essentially the same as find file however it only checks for
 * directories. Therefore any input given the code will automatically add a 
 * leading slash to the users input. "tools becomes /tools" then it returns
 * all directories that contain that in the deepest subdirectory of the tree
 * 
 * FIND REGEX
 * find regex works on basic regular expression pattern matching. This will
 * match any pattern the user inputs and not only those in the deepest subdirectory
 * 
 * POSSIBLE IMPROVEMENTS
 *  - optimize search time complexity by reducing the amount of files to parse
 *      from the query
 *  - add in more features such as zip, and stat view
 * 
 * KNOWN BUGS
 *  - File search may hang on very large queries
 * 
 * Version 1.0 - basic functions implemented
 ******************************************************************************/
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
#include "util.h"
#include "date.h"
#include <regex.h>

//global variables
sqlite3 *db;
char* sql;
int remoteConnection;
char* diskName;
char* databaseName;
int TOTAL_DISKS = 17;
struct flags{
        bool force;
        int max;
        bool match;
        std::string disk;
        std::string user;
};
int differenceInDays = 0;

///<summary>
/// disk name helper allows the code to determine which
/// database files to search. This works on the opposite
/// databases as index.cpp to prevent corrupting database
/// disk images
///</summary>
std::string diskNameHelper(int i){
    std::string disc = "sc_aepdeD";
    if(differenceInDays %2 == 1){
        if(i < 10){
            disc += "0" + std::to_string(i) + ".db";
        }else{
            disc += std::to_string(i) + ".db";
        }
    }else{
        if(i < 10){
            disc += "0" + std::to_string(i) + "odd.db";
        }else{
            disc += std::to_string(i) + "odd.db";
        }
    }
    return disc;
}

///<summary>
/// This method is called from find file
/// and is resposible for printing results to
/// the display
///@params
/// sqlite3_stmt *&stmt -> pointer to the address of the sql statement
/// std::string &filename -> reference to the filename string
/// int i -> the disk number currently being searched (0) if d flag
/// bool isLoop -> true if d flag is false
/// std::string diskName -> name of the disk
/// int max -> max files to be printed
/// bool match -> match flag from user
/// std::string &username -> reference to username if -u flag is true
///</summary>
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
            }else{
                continue;
            }
        }else{
            if(file == filename){
                std::cout << tmp_name << std::endl;
                count++;
            }else{
                continue;
            }
        }
    }
    sqlite3_reset(stmt);
    sqlite3_close(db);
}

///<summary>
/// code snippet taken from https://regex-generator.olafneumann.org/
///</summary>
int useRegex(char* textToCheck, const char* regexPattern) {
    regex_t compiledRegex;
    int reti;
    int actualReturnValue = -1;
    char messageBuffer[100];

    /* Compile regular expression */
    reti = regcomp(&compiledRegex, regexPattern, REG_EXTENDED | REG_ICASE);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        return -2;
    }

    /* Execute compiled regular expression */
    reti = regexec(&compiledRegex, textToCheck, 0, NULL, 0);
    if (!reti) {
        std::cout << textToCheck << std::endl;
        actualReturnValue = 0;
    } else if (reti == REG_NOMATCH) {
        actualReturnValue = 1;
    } else {
        regerror(reti, &compiledRegex, messageBuffer, sizeof(messageBuffer));
        fprintf(stderr, "Regex match failed: %s\n", messageBuffer);
        actualReturnValue = -3;
    }

    /* Free memory allocated to the pattern buffer by regcomp() */
    regfree(&compiledRegex);
    return actualReturnValue;
}

///<summary>
/// This method is called from find regex
/// and is resposible for printing results to
/// the display
///@params
/// sqlite3_stmt *&stmt -> pointer to the address of the sql statement
/// std::string &filename -> reference to the filename string
/// int i -> the disk number currently being searched (0) if d flag
/// bool isLoop -> true if d flag is false
/// std::string diskName -> name of the disk
/// int max -> max files to be printed
/// std::string &username -> reference to username if -u flag is true
///</summary>
void print_regex_result(sqlite3_stmt *&stmt, std::string &regex, int i, 
                        bool isLoop, std::string diskName, int max, std::string &username){
    std::string disc;
    if(isLoop)
        disc = diskNameHelper(i);
    else
        disc = diskName;
    databaseName = const_cast<char*>(disc.c_str());
    remoteConnection = sqlite3_open(databaseName, &db);
    const char* pattern = regex.c_str();
    std::string query;
    if(username == ""){ 
        query = "SELECT name from FILES;";
    }
    else{
        std::string a = "printf \'%s\' \"" + username + "\"| md5sum";
        const char* ha = const_cast<char*>(a.c_str());
        std::string hash = exec(ha);
        hash = trim(hash);
        int pos = hash.find("-");
        hash = hash.substr(0,pos);
        hash = trim(hash);
        query = "SELECT name from FILES WHERE id=\'" + hash + "\';";
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
        std::string tmp_str = std::string(reinterpret_cast<const char*>(tmp_name));
        char *cstr = &tmp_str[0];
        int j = useRegex(cstr, pattern);
        count = j == 0 ? count += 1 : count = count;
    }
    sqlite3_reset(stmt);
    sqlite3_close(db);
}

///<summary>
/// This method is called from find dir
/// and is resposible for printing results to
/// the display
///@params
/// sqlite3_stmt *&stmt -> pointer to the address of the sql statement
/// std::string &filename -> reference to the dirname string
/// int i -> the disk number currently being searched (0) if d flag
/// bool isLoop -> true if d flag is false
/// std::string diskName -> name of the disk
/// int max -> max files to be printed
/// bool match -> match flag from user
/// std::string &username -> reference to username if -u flag is true
///</summary>
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

///<summary>
/// This method is called from main
/// and is resposible for passing the correct
/// arguments to print
///@params
/// std::string filename ->  filename string
/// std::string disk -> name of the disk
/// int max -> max files to be printed
/// bool match -> match flag from user
/// std::string user -> username if -u flag is true
///</summary>
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

///<summary>
/// This method is called from main
/// and is resposible for passing the correct
/// arguments to print
///@params
/// std::string dirname ->  dirname string
/// std::string disk -> name of the disk
/// int max -> max files to be printed
/// bool match -> match flag from user
/// std::string user -> username if -u flag is true
///</summary>
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

///<summary>
/// This method is called from main
/// and is resposible for passing the correct
/// arguments to print
///@params
/// std::string regex ->  regex string
/// std::string disk -> name of the disk
/// int max -> max files to be printed
/// bool match -> match flag from user
/// std::string user -> username if -u flag is true
///</summary>
void find_regex(std::string regex, bool force, int maximum, std::string disk, std::string user){
    sqlite3_stmt *stmt;
    if(disk == ""){
        if(force){
            int limit = std::numeric_limits<int>::max();
            for(int i = 1; i < TOTAL_DISKS + 1; i++){
                print_regex_result(stmt, regex, i, true, "", limit, user);
            }
            sqlite3_finalize(stmt);
        }else{
            for(int i = 1; i < TOTAL_DISKS + 1; i++){
                print_regex_result(stmt, regex, i, true, "", maximum, user);
            }
            sqlite3_finalize(stmt);
        }
    }else{
        if(force){
            int limit = std::numeric_limits<int>::max();
            print_regex_result(stmt, regex, 0, false, disk, limit, user);
        }else{
            print_regex_result(stmt, regex, 0, false, disk, maximum, user);
        }
    }
    return;
}

///<summary>
/// parses the flags from the command line
/// @params
/// int argc -> number of arguments
/// char** argv -> pointer to the pointer of characters from the cmd line
/// flags f -> reference to flags struct
/// Parser p -> pointer to the reference of the parser object
///</summary>
int setFlags(int argc, char** argv, flags &f, Parser *&p){
    if(p->findFlag("-f")){
            f.force = true;
    }
    if(p->findFlag("-x")){
        for(int i = 2; i < argc; i++){
            if(std::string(argv[i]) == "-x"){
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
    if(p->findFlag("-m")){
        f.match = true;
    }
    if(p->findFlag("-d")){
        for(int i = 2; i < argc; i++){
            if(std::string(argv[i]) == "-d"){
                try{
                    std::string str(argv[i+1]);
                    int num = std::stoi(str);
                    std::string diskNum = num < 10 ? "0" + std::to_string(num) : std::to_string(num);
                    std::string diskDb;
                    if(differenceInDays % 2 == 1)
                        diskDb = "sc_aepdeD" + diskNum + ".db";
                    else
                        diskDb = "sc_aepdeD" + diskNum + "odd.db";
                    f.disk = diskDb;
                }catch(...){
                    std::cout << "Error... please specify disk number after disk flag\n";
                    return 3;
                }
            }
        }
    }
    if(p->findFlag("-u")){
        for(int i = 2; i < argc; i++){
            if(std::string(argv[i]) == "-u"){
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

///<summary>
/// main method
///</summary>
int main(int argc, char** argv){
    Parser *p = new Parser(argc, argv);
    p->setFileName("./indexer.cpp");
    p->addArgument("--file", "Given a file name, query results that contain that file name\n\tand display its path on the disk (Note: use \'_\' to display all files)\n\tThis is the default if no argument is given");
    p->addArgument("--dir", "Given a directory name, query results that contain that directory name\n\tand display its path on the disk (Note: use \'_\' to display all directories)");
    p->addArgument("-r", "Given a regular expression, query results that match the regex\n\tand display its path on the disk. This option will give more precise results\n\thowever it runs slower on large disks. Limit disk and user if possible when using this option.\n\tIn addition, make sure to surround your expression in quotes as shown in this example here:\n\t\"./indexer -r \'/nfs/site/disks/sc_aepdeD02/jsl/tvpv/trc/d01/s99/([a-zA-Z]+([0-9]+[a-zA-Z]+)+)\n\t_00_([a-zA-Z]+([0-9]+[a-zA-Z]+)+)_([a-zA-Z]+([0-9]+[a-zA-Z]+)+)_/([a-zA-Z]+([0-9]+[a-zA-Z]+)+)\n\t_00_VMB022TS_exnm3zADA000013S_([a-zA-Z]+([0-9]+[a-zA-Z]+)+)_x+_00_all_idcode_emu\\.itpp\\.gz\\.bz2\'\n\tFor help generating working regex patterns visit \'https://regex-generator.olafneumann.org/\'");
    p->addFlag("-f", "FORCE: If force is set then all query results will display without any pause up this flag is true by default");
    p->addFlag("-x", "MAX: If the max flag is given, specify the amount of results to display to the screen\n\tbefore a pause is shown");
    p->addFlag("-m", "MATCH: If the match flag is given, results will only display names that exactly\n\tmatch the name of the given token");
    p->addFlag("-d", "DISK: If the disk flag is given, results will be narrowed down to one\n\tdisk instead of all disks. (This can be useful to prevent hangs on large queries)\n\tPlease enter the disk number and do not manually enter a leading zero\n\ti.e \"--disk 2\" will search sc_aepdeD02.db, \"--disk 02\" will fail");
    p->addFlag("-u", "USER: If the user flag is given, results will only be displayed for a username\n\tentered after the flag. (Note: if you want to search for files not owned\n\tby a particular user enter \'root_scaepdeD(DISKNUM)\' where DISKNUM is\n\tthe number of the disk you are searching (i.e 02)");


    //IMPORTANT DONT CHANGE FRON CRON JOB
    Date dateOfCreation;
    dateOfCreation.d = 13;
    dateOfCreation.m = 7;
    dateOfCreation.y = 2022;

    std::string a = exec("date \'+%d\'");
    a = trim(a);
    int day = stoi(a);
    a = exec("date \'+%m\'");
    a = trim(a);
    int month = stoi(a);
    a = exec("date \'+%Y\'");
    a = trim(a);
    int year = stoi(a);
    Date today;
    today.d = day;
    today.m = month;
    today.y = year;
    differenceInDays = abs(getDifference(today,dateOfCreation));

    if(argc < 2){
        p->printHelp();
        return 0;
    }

    
    flags f;
    f.force = true;
    f.match = false;
    f.max = 50;
    f.disk = "";
    f.user = "";
    int foobar = setFlags(argc, argv, f, p);

    if(foobar == 0){
        if(std::string(argv[1]) == "--file"){
            if(argc < 3){
                std::cout << "Error [--file]... please specify file name to search for, followed by flags\n";
                return 1;
            }
            std::string filename = argv[2];
            find_file(filename, f.force, f.max, f.match, f.disk, f.user);
        }else if(std::string(argv[1]) == "-d"){
            if(argc < 3){
                std::cout << "Error [--dir]... please specify directory to search for, followed by flags\n";
                return 1;
            }
            std::string dirname = argv[2];
            find_dir(dirname, f.force, f.max, f.match, f.disk, f.user);
        }else if(std::string(argv[1]) == "-r"){
            if(argc < 3){
                std::cout << "Error [-r]... please specify regex to search for, followed by flags\n";
                return 1;
            }
            if(f.match){
                std::cout << "Error cannot specify the \'--match\' flag when using regular expressions\n";
                return 1;
            }
            std::string regex = argv[2];
            find_regex(regex, f.force, f.max, f.disk, f.user);
        }else if(std::string(argv[1]) == "-h"){
            p->printHelp();
        }else{
            std::string filename = std::string(argv[1]);
            find_file(filename, f.force, f.max, f.match, f.disk, f.user);
        }
    }else{
        p->printHelp();
        return 1;
    }

    delete p;
    return 0;
}