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
#include "util.h"

sqlite3 *db;
char* sql;
char *zErrMsg = 0;
int remoteConnection;
char* diskName;
char* databaseName;
std::string _id;
std::string userName;
std::vector<std::string> userHashes;
std::string rootHash;

std::vector<std::string> split_string_by_newline(std::string path, std::string &str, std::string delim){
    std::vector<std::string> result;
    int start = 0;
    int end = str.find(delim);
    while(end != -1){
        result.push_back(path + str.substr(start+1, end - start-1));
        start = end + delim.size();
        end = str.find(delim, start);
    }
    result.push_back(str.substr(start, end-start));

    return result;
}


void list_dir(const char *path, std::vector<std::string> &dirs) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if(strcmp (entry->d_name,".") != 0 && strcmp (entry->d_name,"..") != 0 && strcmp(entry->d_name,".snapshot") != 0){
            std::string tmp(entry->d_name);
            dirs.push_back((tmp));
        }
    }

    closedir(dir);
}

std::string get_path_string(char* disk){
    std::string path = "/nfs/site/disks/";
    std::string disc(disk);
    path += disc + "/";
    return path;
}

inline size_t isUser(std::string s){
    std::string isuser = "/user";
    return s.find(isuser);
}

bool isSnapshot(std::string s){
    return s.find("/.snapshot/") != std::string::npos || s.find("/archivelog/") != std::string::npos;
}

static int get_info(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
    std::string _fpath(fpath);

    if(!isSnapshot(_fpath)){
        std::string type = (tflag == FTW_D) ?   "d"   : (tflag == FTW_DNR) ? "dnr" :
            (tflag == FTW_DP) ?  "dp"  : (tflag == FTW_F) ?   "f" :
            (tflag == FTW_NS) ?  "ns"  : (tflag == FTW_SL) ?  "sl" :
            (tflag == FTW_SLN) ? "sln" : "???";

        std::stringstream ss;
        ss << fpath, ftwbuf->base;
        bool f = isUser(ss.str()) == SIZE_MAX ? false : true;
        std::string idx = ss.str();
        std::string op;
        std::string root;
        if(!f){
            if(type=="f"){
                root = "INSERT OR IGNORE INTO FILES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + idx +"\');";
                char *sqlInsert = const_cast<char*>(root.c_str());
                remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
                op = "inserted into FILES values : " + rootHash + " " + idx;            
            }else{
                root = "INSERT OR IGNORE INTO DIRECTORIES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + idx +"\');";
                char *sqlInsert = const_cast<char*>(root.c_str());
                remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
                op = "inserted into DIRECTORIES values : " + rootHash + " " + idx;
            }
            wasSQLOperationSuccessful(remoteConnection, root);
        }else{
            if(type=="f"){
                root = "INSERT OR IGNORE INTO FILES (ID,NAME) VALUES(\'" + _id + "\',\'" + idx +"\');";
                char *sqlInsert = const_cast<char*>(root.c_str());
                remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
                op = "inserted into FILES values : " + _id + " " + idx;            
            }else{
                root = "INSERT OR IGNORE INTO DIRECTORIES (ID,NAME) VALUES(\'" + _id + "\',\'" + idx +"\');";
                char *sqlInsert = const_cast<char*>(root.c_str());
                remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
                op = "inserted into DIRECTORIES values : " + _id + " " + idx;
            }
            wasSQLOperationSuccessful(remoteConnection, root);
        }
        return 0; /* To tell nftw() to continue */
    }else{
        return 2;
    }       
}

/*************************************
 * initalize the database and users
**************************************/
void init_db(char* dbName){
    std::cout << "Initalizing databse...\n";
    
    if(remoteConnection){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }
    sql = "DROP TABLE IF EXISTS USERS";
    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Users table creation");
    sql = "DROP TABLE IF EXISTS DIRECTORIES";
    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Users table creation");
    sql = "DROP TABLE IF EXISTS FILES";
    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Users table creation");

    sql = "CREATE TABLE IF NOT EXISTS USERS("\
                "ID TEXT PRIMARY KEY NOT NULL," \
                "NAME       TEXT    NOT NULL," \
                "DISKUSAGE  INT);";

    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Users table creation");

    sql = "CREATE TABLE IF NOT EXISTS DIRECTORIES("\
        "ID     TEXT     NOT NULL,"\
        "NAME   TEXT    NOT NULL UNIQUE,"\
        "SIZE   INT,"\
        "FOREIGN KEY(ID) REFERENCES USERS(ID));";

    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Directory table creation");

    sql = "CREATE TABLE IF NOT EXISTS FILES("\
        "ID     TEXT     NOT NULL,"\
        "NAME   TEXT    NOT NULL UNIQUE,"\
        "SIZE   INT,"\
        "LASTACCESS TEXT,"\
        "FOREIGN KEY(ID) REFERENCES USERS(ID));";

    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "File table creation");
}


void index_users(char* dbName, char* disk){
    std::cout << "Indexing users...\n";
    //get path to disk
    std::string path = get_path_string(disk);
    path += "user";

    //create a const char * and char* of path for method calls
    const char* constBuf = path.c_str();
    try
    {
        chdir(constBuf);
    }
    catch(const std::exception& e)
    {
        std::string buf = path;
        buf.append(disk);
        buf.append("/users");
        constBuf = buf.c_str();
        chdir(constBuf);
    }

    //list dirs in path
    std::vector<std::string> user_dirs;
    list_dir(constBuf, user_dirs);


    std::string r = "root_";
    r.append(disk);
    std::string a = "printf \'%s\' \"" + r + "\"| md5sum";
    const char* ha = const_cast<char*>(a.c_str());
    std::string hash = exec(ha);
    hash = trim(hash);
    int pos = hash.find("-");
    hash = hash.substr(0,pos);
    hash = trim(hash);
    rootHash = hash;
    userHashes.push_back(rootHash);

    std::string root = "INSERT OR REPLACE INTO USERS (ID,NAME) VALUES(\'" + hash + "\',\'" + r +"\');"; 
    char *sqlInsert = const_cast<char*>(root.c_str());
    remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, root);


    for(auto dir : user_dirs){
        std::string tmp(dir);
        a = "printf \'%s\' \"" + tmp + "\"| md5sum";
        const char* has = const_cast<char*>(a.c_str());
        hash = exec(has);
        hash = trim(hash);
        int pos = hash.find("-");
        hash = hash.substr(0,pos);
        hash = trim(hash);
        userHashes.push_back(hash);
        std::string insert = "INSERT OR REPLACE INTO USERS (ID,NAME) VALUES (\'" + hash + "\',\'" + tmp + "\');";
        char *sqlInsert = const_cast<char*>(insert.c_str());
        remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
        std::string op = "inserted into users values : " + hash + " " + tmp;
        wasSQLOperationSuccessful(remoteConnection, insert);
    }
}



void run_multi_threaded(){
    std::string tmp(diskName);
    tmp += ".db";
    databaseName = const_cast<char*>(tmp.c_str());

    remoteConnection = sqlite3_open(databaseName, &db);

    std::cout << "Fresh indexing for disk " + tmp + " asynchronously. This may take some time...\n";
    std::thread initalizer(init_db,databaseName);

    initalizer.join();

    std::thread user_thread(index_users, databaseName, diskName);

    user_thread.join();

    std::string disk_path = get_path_string(diskName);

    // chdir(const_cast<char*>(disk_path.c_str()));
    std::vector<std::string> dirs;
    list_dir(const_cast<char*>(disk_path.c_str()), dirs);

    int flags = 0;
    std::cout << "Indexing files and dirs\n";
    for(auto d : dirs){
        if(d != "user" && d != "users"){
            std::string tmp = disk_path + d;
            auto temp = tmp.c_str();
            int i = nftw(temp, get_info, 20, flags);
        }else{
            sqlite3_stmt *stmt;
            std::string tmp = disk_path + d;
            auto temp = tmp.c_str();
            std::vector<std::string> usrs;
            list_dir(temp, usrs);
            for(auto usr : usrs){
                std::string user(usr);
                userName = user;
                std::string query = "SELECT ID FROM USERS WHERE NAME=\'" + user + "\';";
                const char* sqlSelect = query.c_str();
                sqlite3_prepare_v2(db, sqlSelect, -1, &stmt, 0);
                const unsigned char* tmp_id;   
                while(sqlite3_step(stmt) != SQLITE_DONE){
                    tmp_id = sqlite3_column_text(stmt, 0);
                    _id = std::string(reinterpret_cast<const char*>(tmp_id));
                }
                sqlite3_reset(stmt);
                std::string tmp2 = tmp + "/" + user;
                auto temp2 = tmp2.c_str();
                int i = nftw(temp2, get_info, 20, flags);
            }
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_close(db);
    return;
}

int main(int argc, char** argv){
    struct timespec begin, end; 
    clock_gettime(CLOCK_REALTIME, &begin);

    if(argc >= 2){
        diskName = argv[1];
        run_multi_threaded();
    }else{
        std::cout << "Please enter the name of the disk you wish to index\n";
    }

    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    printf("Time measured: %.3f seconds.\n", elapsed);

    return 0;
}