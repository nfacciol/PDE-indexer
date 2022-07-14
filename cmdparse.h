/******************************************************************************
* Author: Nicholas Facciola
* 
* cmdparse h defines the methods and data that will be implemented by
* cmdparse cpp
*******************************************************************************/
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

class Parser{
    protected:
        struct argument{
            std::string tokenName;
            std::string tokenDesc;
        };
        std::vector<argument> arguments;
        std::vector<std::string> tokens;
        std::string programDescription;
        std::string fileName;
        std::vector<argument> flags;

    public:
        Parser(int &argc, char **argv){
            for(int i=1; i < argc; ++i){
                this->tokens.push_back(std::string(argv[i]));
            }
        }
        void printTokens();
        void printHelp();
        void setFileName(std::string f);
        void addArgument(std::string name, std::string desc);
        void addFlag(std::string name, std::string desc);
        bool findFlag(std::string flagName);
        std::string getFileName();
        std::vector<std::string> addGroup(std::string groupName);
};