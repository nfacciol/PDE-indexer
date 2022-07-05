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
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

//global variables
sqlite3 *db;
char* sql;
char *zErrMsg = 0;
int remoteConnection;
char* diskName;
char* databaseName;
const std::string WHITESPACE = " \n\r\t\f\v";
struct User{
    string userID;
    string userName;
    string topLevelDirectory;
    vector<string> userFiles;
    vector<string> userDirs;
};
struct node
{
  string name;   
  string type; 
};
static bool isAsync = false;
string rootHash;
struct sysinfo memInfo;

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

void wasSQLOperationSuccessful(int &rc, string operation){
    if (rc != SQLITE_OK){
        cout << operation << endl;
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    else {
        //cout << operation + " completed successfully\n";
    }
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

string exec(const char* cmd){
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

vector<string> split_string_by_newline(string path, string &str, string delim){
    vector<string> result;
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

void get_directory_vector(vector<char *>&vect, const char *dirToRead){
    DIR *dir;
    struct dirent *diread;

    if((dir = opendir(dirToRead)) != nullptr){
        while((diread = readdir(dir)) != nullptr){
            if(strcmp (diread->d_name,".") != 0 && strcmp (diread->d_name,"..") != 0 && strcmp(diread->d_name,".snapshot") != 0){ //uneeded dirs
                vect.push_back(diread->d_name);
            }
        }
        closedir(dir);
    }else{
        perror("opendir");
        return;
    }
}

void list_dir(const char *path, vector<string> &dirs) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if(strcmp (entry->d_name,".") != 0 && strcmp (entry->d_name,"..") != 0 && strcmp(entry->d_name,".snapshot") != 0){
            string tmp(entry->d_name);
            dirs.push_back((tmp));
        }
    }

    closedir(dir);
}


void get_all_dir_and_file(const char *name, vector<string> &_files, vector<string> &dirs, bool lastSlash, bool isUser)
{

    cout << "Virtual mem:\t" << getValueVirtual() << "MB" << endl;
    cout << "Physical mem:\t" << getValuePhysical() << "MB" << endl;

    DIR *dir;
    struct dirent *entry;
 
    if (!(dir = opendir(name))) 
    {  
        //cout << "Cant open " << name << endl;
        return;
    }
    if (!(entry = readdir(dir)))
    {
        //cout << "can't read " << name << endl;
        return;
    }

    do 
    {
        string slash="";
        if(!lastSlash)
          slash = "/";
      
        string parent(name);
        string file(entry->d_name);
        string final = parent + slash + file;


        if (entry->d_type == DT_DIR) //its a directoru
        {
            //skip the . and .. directory
            if(isUser){
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".snapshot") == 0)
                    continue;
            }else{
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".snapshot") == 0 
                || strcmp(entry->d_name, "user") == 0 || strcmp(entry->d_name, "users") == 0)
                    continue;
            }
            struct node tmp;
            tmp.name = final;
            tmp.type = "D";
            dirs.push_back(tmp.name);
            //ans.push_back(tmp);
            get_all_dir_and_file(final.c_str(), _files, dirs, false, isUser);
        } 
        else //its a file  
        {
            struct node tmp;
            tmp.name = final;
            tmp.type = "F";
            _files.push_back(tmp.name);
        }
    } while (entry = readdir(dir));
    closedir(dir);
}


string get_path_string(char* disk){
    string path = "/nfs/site/disks/";
    string disc(disk);
    path += disc;
    return path;
}


/************
 * initalize the database
*************/
void init_db(char* dbName){
    cout << "Initalizing databse...\n";
    

    if(remoteConnection){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    sql = "DROP table IF EXISTS USERS";
    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Users table drop");

    sql = "DROP table IF EXISTS DIRECTORIES";
    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Directories table drop");

    sql = "DROP table IF EXISTS FILES";
    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Files table drop");

    sql = "CREATE TABLE USERS("\
                "ID TEXT PRIMARY KEY NOT NULL," \
                "NAME       TEXT    NOT NULL," \
                "DISKUSAGE  INT);";

    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Users table creation");

    sql = "CREATE TABLE DIRECTORIES("\
        "ID     TEXT     NOT NULL,"\
        "NAME   TEXT    NOT NULL,"\
        "SIZE   INT,"\
        "FOREIGN KEY(ID) REFERENCES USERS(ID));";

    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "Directory table creation");

    sql = "CREATE TABLE FILES("\
        "ID     TEXT     NOT NULL,"\
        "NAME   TEXT    NOT NULL,"\
        "SIZE   INT,"\
        "LASTACCESS TEXT,"\
        "FOREIGN KEY(ID) REFERENCES USERS(ID));";

    remoteConnection = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    wasSQLOperationSuccessful(remoteConnection, "File table creation");
}


void index_users(char* dbName, char* disk){
    cout << "Indexing users...\n";
    //get path to disk
    string path = "/nfs/site/disks/";
    string buf(path);
    buf.append(disk);
    buf.append("/user");

    //create a const char * and char* of path for method calls
    const char* constBuf = buf.c_str();
    try
    {
        chdir(constBuf);
    }
    catch(const std::exception& e)
    {
        buf = path;
        buf.append(disk);
        buf.append("/users");
        constBuf = buf.c_str();
        chdir(constBuf);
    }

    //list dirs in path
    DIR *dir;
    struct dirent *diread;
    vector<char *> user_dirs;

    if((dir = opendir(constBuf)) != nullptr){
        while((diread = readdir(dir)) != nullptr){
            if(strcmp (diread->d_name,".") != 0 && strcmp (diread->d_name,"..") != 0){ //uneeded dirs
                user_dirs.push_back(diread->d_name);
            }
        }
        closedir(dir);
    }else{
        perror("opendir");
        return;
    }

    string r = "root_";
    r.append(disk);
    string a = "printf \'%s\' \"" + r + "\"| md5sum";
    const char* ha = const_cast<char*>(a.c_str());
    string hash = exec(ha);
    hash = trim(hash);
    int pos = hash.find("-");
    hash = hash.substr(0,pos);
    hash = trim(hash);
    rootHash = hash;

    string root = "INSERT OR REPLACE INTO USERS (ID,NAME) VALUES(\'" + hash + "\',\'" + r +"\');"; 
    char *sqlInsert = const_cast<char*>(root.c_str());
    remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
    string op = "inserted into users values : " + hash + " " + r;
    cout << op << endl;
    wasSQLOperationSuccessful(remoteConnection, root);


    for(auto dir : user_dirs){
        string tmp(dir);
        a = "printf \'%s\' \"" + tmp + "\"| md5sum";
        const char* has = const_cast<char*>(a.c_str());
        hash = exec(has);
        hash = trim(hash);
        int pos = hash.find("-");
        hash = hash.substr(0,pos);
        hash = trim(hash);
        string insert = "INSERT OR REPLACE INTO USERS (ID,NAME) VALUES (\'" + hash + "\',\'" + tmp + "\');";
        char *sqlInsert = const_cast<char*>(insert.c_str());
        remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
        string op = "inserted into users values : " + hash + " " + tmp;
        cout << op << endl;
        wasSQLOperationSuccessful(remoteConnection, insert);
    }
}


void index_files(char* dbName, const char* diskPath, vector<string> files, vector<User> users){
    cout << "Indexing files...\n";

    //root section
    // for(auto dir : dirs){
    //     if(dir != "user" && dir != "users"){
    //         string tmp(diskPath);
    //         tmp.append(dir);
    //         string command = "nice find " + tmp + " -type f | grep -v \'Permission denied\'";
    //         const char* cmd = const_cast<char *>(command.c_str());
    //         string b = exec(cmd);
    //         vector<string> results = split_string_by_newline("", b, "\n");
    //         for(auto result : results){
    //             string query = "INSERT OR REPLACE INTO FILES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + result + "\');";
    //             char *sqlInsert = const_cast<char*>(query.c_str());
    //             remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
    //             string op = "inserted into files values : " + to_string(0) + " " + result;
    //             wasSQLOperationSuccessful(remoteConnection, query);
    //         }
    //     }
    // }
    for(auto f : files){
        string query = "INSERT OR REPLACE INTO FILES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + f + "\');";
        char *sqlInsert = const_cast<char*>(query.c_str());
        remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
        string op = "inserted into files values : " + rootHash + " " + f;
        wasSQLOperationSuccessful(remoteConnection, query);
    }

    ///users section
    for(auto u :users){
        string id = u.userID;
        for(auto file : u.userFiles){
            string query = "INSERT OR REPLACE INTO FILES (ID,NAME) VALUES(\'" + id + "\',\'"  + file + "\');";
            char *sqlInsert = const_cast<char*>(query.c_str());
            remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
            string op = "inserted into files values : " + id + " " + file;
            wasSQLOperationSuccessful(remoteConnection, query);
        }
    }
}

void index_directories(char* dbName, const char* diskPath, vector<string> dirs, vector<User> users){
    cout << "Indexing directories...\n";

    // for(auto dir : dirs){
    //     if(dir != "user" && dir != "users"){
    //         string tmp(diskPath);
    //         tmp.append(dir);
    //         string command = "nice find " + tmp + " -type d | grep -v \'Permission denied\'";
    //         const char* cmd = const_cast<char *>(command.c_str());
    //         string b = exec(cmd);
    //         vector<string> results = split_string_by_newline("", b, "\n");
    //         for(auto result : results){
    //             string query = "INSERT OR REPLACE INTO DIRECTORIES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + result + "\');";
    //             char *sqlInsert = const_cast<char*>(query.c_str());
    //             remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
    //             string op = "inserted into directories values : " + to_string(0) + " " + result;
    //             wasSQLOperationSuccessful(remoteConnection, query);
    //         }
    //     }
    // }
    for(auto d : dirs){
        string query = "INSERT OR REPLACE INTO DIRECTORIES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + d + "\');";
        char *sqlInsert = const_cast<char*>(query.c_str());
        remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
        string op = "inserted into directories values : " + rootHash + " " + d;
        wasSQLOperationSuccessful(remoteConnection, query);
    }

    ///users section
    for(auto u :users){
        string id = u.userID;
        for(auto d : u.userDirs){
            string query = "INSERT OR REPLACE INTO DIRECTORIES (ID,NAME) VALUES(\'" + id + "\',\'"  + d + "\');";
            char *sqlInsert = const_cast<char*>(query.c_str());
            remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
            string op = "inserted into files directories : " + id + " " + d;
            wasSQLOperationSuccessful(remoteConnection, query);
        }
    }

}

void index_directories_and_files(char* dbName, char* disk){
    cout << "Indexing directories and files...\n";
    //get path to disk
    string path = "/nfs/site/disks/";
    string buf(path);
    buf.append(disk);

    //create a const char * and char* of path for method calls
    const char* constBuf = buf.c_str();
    chdir(constBuf);
    // char* starBuf = new char[buf.length() + 1];
    // copy(buf.begin(), buf.end(), starBuf);
    
    // //goto path
    // getcwd(starBuf, sizeof(starBuf));


    //list dirs in path
    vector<char *> dirs;
    get_directory_vector(dirs, constBuf);

    for(auto dir : dirs){
        if(strcmp(dir,"user") != 0){
            string tmp = buf + "/" + dir;
            const char* temp = tmp.c_str();
            chdir(temp);
            //insert top level first
            string query = "INSERT OR REPLACE INTO DIRECTORIES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + tmp + "\');";
            char *sqlInsert = const_cast<char*>(query.c_str());
            remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
            string op = "inserted into directories values : " + rootHash + " " + tmp;
            wasSQLOperationSuccessful(remoteConnection, query);
            //now insert all of trees children
            string a = exec("nice find . -type d | grep -v \'Permission denied\'");
            vector<string> results = split_string_by_newline(tmp, a, "\n");
            for(auto result : results){
                string query = "INSERT OR REPLACE INTO DIRECTORIES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + result + "\');";
                char *sqlInsert = const_cast<char*>(query.c_str());
                remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
                string op = "inserted into directories values : " + to_string(0) + " " + result;
                wasSQLOperationSuccessful(remoteConnection, op);
            }
            results.clear();
            string b = exec("nice find . -type f | grep -v \'Permission denied\'");
            results = split_string_by_newline(tmp, b, "\n");
            for(auto result : results){
                string query = "INSERT OR REPLACE INTO FILES (ID,NAME) VALUES(\'" + rootHash + "\',\'" + result + "\');";
                char *sqlInsert = const_cast<char*>(query.c_str());
                remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
                string op = "inserted into files values : " + rootHash + " " + result;
                wasSQLOperationSuccessful(remoteConnection, query);
            }
            
        }

    }

    ///users section

    sqlite3_stmt *stmt;
    string diskStr(disk);
    string tmp = "/nfs/site/disks/" + diskStr + "/user";
    const char* temp = tmp.c_str();
    try
    {
        chdir(temp);
    }
    catch(const std::exception& e)
    {
        tmp = "/nfs/site/disks/" + diskStr + "/users";
        temp = tmp.c_str();
        chdir(temp);
    }
    
    

    vector<char *> usrs;
    get_directory_vector(usrs, temp);

    vector<User> userDetails;
    
    for(auto usr : usrs){
        string user(usr);
        string query = "SELECT ID FROM USERS WHERE NAME=\'" + user + "\';";
        const char* sqlSelect = query.c_str();
        sqlite3_prepare_v2(db, sqlSelect, -1, &stmt, 0);
        int tmp_id;
        while(sqlite3_step(stmt) != SQLITE_DONE){
            tmp_id = sqlite3_column_int(stmt, 0);
        }
        sqlite3_reset(stmt);
        string tmp2 = tmp + "/" + user;

        User u;
        u.userID = tmp_id;
        u.userName = user;
        u.topLevelDirectory = tmp2;
        userDetails.push_back(u);
        sqlite3_finalize(stmt);
    }

    for(auto u : userDetails){
        chdir(u.topLevelDirectory.c_str());
        string query = "INSERT OR REPLACE INTO DIRECTORIES (ID,NAME) VALUES(\'" + u.userID + "\',\'" + u.topLevelDirectory + "\');";
        char *sqlInsert = const_cast<char*>(query.c_str());
        remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
        string op = "inserted into directories values : " + u.userID + " " + u.topLevelDirectory;
        wasSQLOperationSuccessful(remoteConnection, query);
        //now insert all of trees children
        string a = exec("find . -type d");
        vector<string> results = split_string_by_newline(u.topLevelDirectory, a, "\n");
        for(auto result : results){
            string query = "INSERT OR REPLACE INTO DIRECTORIES (ID,NAME) VALUES(\'" + u.userID + "\',\'"  + result + "\');";
            char *sqlInsert = const_cast<char*>(query.c_str());
            remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
            string op = "inserted into directories values : " + u.userID + " " + result;
            wasSQLOperationSuccessful(remoteConnection, query);
        }
        results.clear();
        string b = exec("find . -type f");
        results = split_string_by_newline(u.topLevelDirectory, b, "\n");
        for(auto result : results){
            string query = "INSERT OR REPLACE INTO FILES (ID,NAME) VALUES(\'" + u.userID + "\',\'"  + result + "\');";
            char *sqlInsert = const_cast<char*>(query.c_str());
            remoteConnection = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
            string op = "inserted into files values : " + u.userID + " " + result;
            wasSQLOperationSuccessful(remoteConnection, query);
        }
    }
    //delete[] starBuf;
}

void run_multi_threaded(){
    isAsync = true;
    string tmp(diskName);
    tmp += ".db";
    databaseName = const_cast<char*>(tmp.c_str());

    remoteConnection = sqlite3_open(databaseName, &db);

    cout << "Fresh indexing for disk " + tmp + " asynchronously. This may take some time...\n";
    thread initalizer(init_db,databaseName);

    initalizer.join();

    thread user_thread(index_users, databaseName, diskName);

    user_thread.join();

    string disk_path = get_path_string(diskName);

    // chdir(const_cast<char*>(disk_path.c_str()));
    // vector<string> dirs;
    // list_dir(const_cast<char*>(disk_path.c_str()), dirs);
    auto cPath = disk_path.c_str();
    vector<string> _files;
    vector<string> dirs;
    get_all_dir_and_file(cPath, _files, dirs, false, false);

    sqlite3_stmt *stmt;
    vector<string> usrs;
    string t = disk_path;
    t.append("/user");
    const char* temp = const_cast<char *>(t.c_str());
    list_dir(temp, usrs);
    vector<User> userDetails;
    for(auto usr : usrs){
        string user(usr);
        string query = "SELECT ID FROM USERS WHERE NAME=\'" + user + "\';";
        const char* sqlSelect = query.c_str();
        sqlite3_prepare_v2(db, sqlSelect, -1, &stmt, 0);
        const unsigned char* tmp_id;
        string _id;
        while(sqlite3_step(stmt) != SQLITE_DONE){
            tmp_id = sqlite3_column_text(stmt, 0);
            _id = string(reinterpret_cast<const char*>(tmp_id));
        }
        sqlite3_reset(stmt);
        string tmp2 = t + "/" + user;

        User u;
        u.userID = _id;
        u.userName = user;
        u.topLevelDirectory = tmp2;
        // string command = "nice find " + u.topLevelDirectory + " -type f | grep -v \'Permission denied\'";
        // const char* cmd = const_cast<char *>(command.c_str());
        // string b = exec(cmd);
        // vector<string> results;
        // results = split_string_by_newline("", b, "\n");
        // string command = "nice find " + u.topLevelDirectory + " -type d | grep -v Permission denied";
        // const char* cmd1 = const_cast<char *>(command.c_str());
        // string a = exec(cmd1);
        // vector<string> results1;
        // results1 = split_string_by_newline("", a, "\n");
        // u.userDirs = results1;
        vector<string> userFiles;
        vector<string> userDirs;
        get_all_dir_and_file(u.topLevelDirectory.c_str(), userFiles, userDirs, false, true);
        u.userDirs = userDirs;
        u.userFiles = userFiles;
        userDetails.push_back(u);
        userFiles.clear();
        userDirs.clear();
        sqlite3_finalize(stmt);
    }


    thread files(index_files, databaseName, const_cast<char*>(disk_path.c_str()), _files, userDetails);
    thread directorys(index_directories, databaseName, const_cast<char*>(disk_path.c_str()), dirs, userDetails);

    directorys.join();
    files.join();

    sqlite3_close(db);
    return;
}

void run_single_threaded(){
    string tmp(diskName);
    tmp += ".db";
    databaseName = const_cast<char*>(tmp.c_str());

    remoteConnection = sqlite3_open(databaseName, &db);

    cout << "Fresh indexing for disk " + tmp + " This may take some time...\n";
    init_db(databaseName);
    index_users(databaseName, diskName);
    index_directories_and_files(databaseName, diskName);

    sqlite3_close(db);
    return;     
}

int main(int argc, char** argv){

    struct timespec begin, end; 
    clock_gettime(CLOCK_REALTIME, &begin);
    
    if (argc >=2){
        diskName = argv[1];
        if(argc == 3){
            if(string(argv[2]) == "--async" || string(argv[2]) == "-a"){
                run_multi_threaded();
            }else{
                run_single_threaded();
            }
        }else{
            run_single_threaded();        
        }
    }else{
        cout << "Please enter the name of the disk you wish to index \n";
    }

    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    printf("Time measured: %.3f seconds.\n", elapsed);

    return 0;
}