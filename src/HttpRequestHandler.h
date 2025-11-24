/**
 * @file HttpRequestHandler.h
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.5
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H

#include <sqlite3.h>

#include "HttpServer.h"
#include "trie.h"

class HttpRequestHandler {
  public:
    HttpRequestHandler(std::string homePath, bool imagemode = 0);
    ~HttpRequestHandler();

    bool handleRequest(std::string url, HttpArguments arguments, std::vector<char>& response);

  private:
    bool serve(std::string path, std::vector<char>& response);
    bool loadVocabularyIntoTrie();

    bool luckyHandler(std::vector<char>& response);
    bool predictHandler(std::vector<char>& response, HttpArguments& arguments);
    bool homePageHandler(std::vector<char>& response);
    bool imageHandler(std::vector<char>& response, HttpArguments& arguments, std::string& url);
    bool searchHandler(std::vector<char>& response, HttpArguments& arguments);

    std::string homePath;
    sqlite3* database;
    sqlite3* database_vocab;
    bool imagemode;
    const char* tableName;
    const char* vocabTableName;
    Trie* trie;
};

#endif
