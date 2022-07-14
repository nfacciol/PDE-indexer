/******************************************************************************
* Author: Nicholas Facciola
* 
* cmdparse cpp implements the methods aformentioned in cmdparse.h
* this praser class is then then used in the indexer in order to 
* read the command line arguments
*******************************************************************************/
#include "cmdparse.h"

void Parser::printTokens(){
    for(auto t : tokens){
        std:: cout << t << std::endl;
    }
}

void Parser::setFileName(std::string f){
    fileName = f;
}
void Parser::addArgument(std::string name, std::string desc){
    argument a;
    a.tokenName = name;
    a.tokenDesc = desc;
    arguments.push_back(a);
}

void Parser::addFlag(std::string name, std::string desc){
    argument a;
    a.tokenName = name;
    a.tokenDesc = desc;
    flags.push_back(a);
}

std::string Parser::getFileName(){
    return fileName;
}

bool Parser::findFlag(std::string flagName){
    bool found = false;
    for(int i = 0; i < this->tokens.size(); i++){
        if(this->tokens[i] == flagName)
            found = true;
    }
    return found;
}

void Parser::printHelp(){
    std::cout << "usage: " << fileName << "\t";
    for(int i = 0; i < arguments.size(); i++){
        std::cout << "[" << arguments[i].tokenName << "]";
        if (i%2==0){
            std::cout << "\n\t\t\t";
        }
    }
    std::cout << "\n";
    std::cout << programDescription << "\n";
    std::cout << "argument descriptions\n";
    for(auto a : arguments){
        std::cout << a.tokenName << "\t" << a.tokenDesc << std::endl;
    }
    std::cout << "\nflags\n";
    for(auto f : flags){
        std::cout << f.tokenName << "\t" << f.tokenDesc << std::endl;
    }
}