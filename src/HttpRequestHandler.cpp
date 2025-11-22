/**
 * @file HttpRequestHandler.h
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include "HttpRequestHandler.h"

#include <unicode/uchar.h>
#include <unicode/ustring.h>

#include <chrono>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>

using namespace std;

HttpRequestHandler::HttpRequestHandler(string homePath, bool imageMode) {
    this->homePath = homePath;

    // Sets up database location and table name
    const char* dbFile = imageMode ? "images.db" : "index.db";
    this->tableName = imageMode ? "images_index" : "webpage_index";

    // Opens database
    if (sqlite3_open(dbFile, &database) != SQLITE_OK) {
        cerr << "Error opening database (" << dbFile << "): " << sqlite3_errmsg(database) << endl;
        database = nullptr;
    } else {
        cout << "Database opened successfully: " << dbFile << endl;
        cout << "Search mode: " << (imageMode ? "IMAGES" : "HTML") << endl;
    }

    // Loads vocabulary into Trie
    cout << "Loading vocabulary into Trie..." << endl;
    trie = new Trie();
    if (loadVocabularyIntoTrie()) {
        cout << "Vocabulary loaded successfully." << endl;
    } else {
        cout << "Failed to load vocabulary." << endl;
    }
}

bool HttpRequestHandler::loadVocabularyIntoTrie() {
    // Checks if database is valid
    if (!database) {
        return false;
    }

    // Pointer for read statement
    sqlite3_stmt* stmt;
    // Statement structure
    string sql = string("SELECT content FROM ") + tableName;

    // Compiles SQL statement
    if (sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(database) << endl;
        return false;
    }

    wstring_convert<codecvt_utf8<char32_t>, char32_t> converter;
    uint32_t pages = 0;
    u32string content;
    u32string word;
    word.reserve(20);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        content.clear();
        word.clear();
        content = converter.from_bytes((char*)sqlite3_column_text(stmt, 0));

        if (content.size() > 0) {
            for (char32_t c : content) {
                if (u_isalpha(c)) {
                    // Builds word character by character
                    word.push_back(u_tolower(c));
                } else if (word.size() >= 3) {
                    // Inserts word into Trie once a non-word character is found
                    trie->insert(word);
                    word.clear();
                }
            }
            if (word.size() >= 3) {
                // Inserts last word if applicable
                trie->insert(word);
                word.clear();
            }
            pages++;
        }
        cout << "Pages processed: " << pages << endl;
    }

    sqlite3_finalize(stmt);
    cout << "Total pages processed: " << pages << endl;
    return true;
}

/**
 * @brief Destroys Handler once no longer used
 */
HttpRequestHandler::~HttpRequestHandler() {
    if (database) {
        sqlite3_close(database);
        cout << "Database closed" << endl;
    }

    delete trie;
    cout << "Trie deleted" << endl;
}

/**
 * @brief Serves a webpage from file
 *
 * @param url The URL
 * @param response The HTTP response
 * @return true URL valid
 * @return false URL invalid
 */
bool HttpRequestHandler::serve(string url, vector<char>& response) {
    // Blocks directory traversal
    // e.g. https://www.example.com/show_file.php?file=../../MyFile
    // * Builds absolute local path from url
    // * Checks if absolute local path is within home path
    auto homeAbsolutePath = filesystem::absolute(homePath);
    auto relativePath = homeAbsolutePath / url.substr(1);
    string path = filesystem::absolute(relativePath.make_preferred()).string();

    if (path.substr(0, homeAbsolutePath.string().size()) != homeAbsolutePath)
        return false;

    // Serves file
    ifstream file(path);
    if (file.fail())
        return false;

    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    response.resize(fileSize);
    file.read(response.data(), fileSize);

    return true;
}

bool HttpRequestHandler::handleRequest(string url,
                                       HttpArguments arguments,
                                       vector<char>& response) {
    //=============== PREDICT HANDLER ===============//
    string predictPage = "/predict";
    if (url.substr(0, predictPage.size()) == predictPage) {
        string query;
        if (arguments.find("q") != arguments.end())
            query = arguments["q"];

        // Collect suggestions from Trie
        size_t numSuggestions = trie->collectSuggestions(query, 10);

        // Build response
        string jsonResponse = "[";

        // Converts suggestions to JSON array
        wstring_convert<codecvt_utf8<char32_t>, char32_t> converter;

        for (size_t i = 0; i < trie->collectWords.size(); i++) {
            // Converts UTF-32 word back to UTF-8
            string suggestion = converter.to_bytes(trie->collectWords[i]);

            jsonResponse += "\"" + suggestion + "\"";

            if (i < trie->collectWords.size() - 1) {
                jsonResponse += ",";
            }
        }
        jsonResponse += "]";

        // Sets response
        response.assign(jsonResponse.begin(), jsonResponse.end());
        return true;
    }
    //=============== SEARCH HANDLER ===============//
    string searchPage = "/search";
    if (url.substr(0, searchPage.size()) == searchPage) {
        string searchString;
        if (arguments.find("q") != arguments.end())
            searchString = arguments["q"];

        // Header

        //=============== OLD HTML HEADER ===============//
        // string responseString = string(
        //             "<!DOCTYPE html>\
// <html>\
// \
// <head>\
//     <meta charset=\"utf-8\" />\
//     <title>EDAoogle</title>\
//     <link rel=\"preload\" href=\"https://fonts.googleapis.com\" />\
//     <link rel=\"preload\" href=\"https://fonts.gstatic.com\" crossorigin />\
//     <link
        //     href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;800&display=swap\"
        //     rel=\"stylesheet\" />\
//     <link rel=\"preload\" href=\"../css/style.css\" />\
//     <link rel=\"stylesheet\" href=\"../css/style.css\" />\
// </head>\
// \
// <body>\
//     <article class=\"edaoogle\">\
//         <div class=\"title\"><a href=\"/\">EDAoogle</a></div>\
//         <div class=\"search\">\
//             <form action=\"/search\" method=\"get\">\
//                 <input type=\"text\" name=\"q\" value=\"" +
        //             searchString +
        //             "\" autofocus>\
//             </form>\
//         </div>\
//         ");

        //=============== NEW HTML HEADER ===============//

        string responseString = string(
            "<!DOCTYPE html>\
<html>\
<head>\
    <meta charset=\"utf-8\" />\
    <title>EDAoogle</title>\
    <link rel=\"preload\" href=\"https://fonts.googleapis.com\" />\
    <link rel=\"preload\" href=\"https://fonts.gstatic.com\" crossorigin />\
    <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;800&display=swap\" rel=\"stylesheet\" />\
    <link rel=\"preload\" href=\"../css/style.css\" />\
    <link rel=\"stylesheet\" href=\"../css/style.css\" />\
    <style>\
        #suggestions {\
            position: absolute;\
            background: white;\
            border: 1px solid #ddd;\
            border-radius: 4px;\
            max-height: 300px;\
            overflow-y: auto;\
            display: none;\
            z-index: 1000;\
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);\
        }\
        .suggestion-item {\
            padding: 10px 15px;\
            cursor: pointer;\
            border-bottom: 1px solid #f0f0f0;\
        }\
        .suggestion-item:hover {\
            background-color: #f5f5f5;\
        }\
    </style>\
    <script>\
        document.addEventListener('DOMContentLoaded', () => {\
            const input = document.querySelector('input[name=\"q\"]');\
            const form = document.querySelector('form');\
            let suggestionsDiv = document.createElement('div');\
            suggestionsDiv.id = 'suggestions';\
            input.parentNode.style.position = 'relative';\
            input.parentNode.appendChild(suggestionsDiv);\
            \
            let debounceTimer;\
            \
            input.addEventListener('input', (e) => {\
                clearTimeout(debounceTimer);\
                \
                const query = e.target.value.trim();\
                \
                if (query.length < 2) {\
                    suggestionsDiv.style.display = 'none';\
                    return;\
                }\
                \
                debounceTimer = setTimeout(async () => {\
                    try {\
                        const response = await fetch('/predict?q=' + encodeURIComponent(query));\
                        const suggestions = await response.json();\
                        \
                        if (suggestions.length > 0) {\
                            suggestionsDiv.innerHTML = '';\
                            suggestions.forEach(suggestion => {\
                                const div = document.createElement('div');\
                                div.className = 'suggestion-item';\
                                div.textContent = suggestion;\
                                div.onclick = () => {\
                                    input.value = suggestion;\
                                    suggestionsDiv.style.display = 'none';\
                                    form.submit();\
                                };\
                                suggestionsDiv.appendChild(div);\
                            });\
                            suggestionsDiv.style.display = 'block';\
                        } else {\
                            suggestionsDiv.style.display = 'none';\
                        }\
                    } catch (error) {\
                        console.error('Error fetching suggestions:', error);\
                    }\
                }, 300);\
            });\
            \
            document.addEventListener('click', (e) => {\
                if (e.target !== input) {\
                    suggestionsDiv.style.display = 'none';\
                }\
            });\
        });\
    </script>\
</head>\
\
<body>\
    <article class=\"edaoogle\">\
        <div class=\"title\"><a href=\"/\">EDAoogle</a></div>\
        <div class=\"search\">\
            <form action=\"/search\" method=\"get\">\
                <input type=\"text\" name=\"q\" value=\"" +
            searchString +
            "\" autofocus>\
            </form>\
        </div>\
        ");

        // YOUR JOB: fill in results

        // Starts timer
        auto startTime = chrono::high_resolution_clock::now();

        float searchTime = 0.0F;
        vector<string> results;

        if (!searchString.empty() && database) {
            // Pointer for read statement
            sqlite3_stmt* stmt;
            // Statement structure
            string sql = string("SELECT path FROM ") + tableName + " WHERE " + tableName +
                         " MATCH ? LIMIT 100;";

            // Compiles SQL statement
            if (sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, searchString.c_str(), -1, SQLITE_TRANSIENT);

                // Iterates through results
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* path = (const char*)sqlite3_column_text(stmt, 0);
                    if (path) {
                        results.push_back(string(path));
                    }
                }

                sqlite3_finalize(stmt);
            }
        }

        // Stops timer
        auto endTime = chrono::high_resolution_clock::now();
        searchTime = chrono::duration<float>(endTime - startTime).count();

        // Print search results

        responseString += "<div class=\"results\">" + to_string(results.size()) + " results (" +
                          to_string(searchTime) + " seconds):</div>";

        for (auto& result : results) {
            // Extracts display name from path
            size_t lastSlash = result.find_last_of('/');
            string displayName =
                (lastSlash != string::npos) ? result.substr(lastSlash + 1) : result;

            // Removes extension
            size_t lastDot = displayName.find_last_of('.');
            if (lastDot != string::npos) {
                displayName = displayName.substr(0, lastDot);
            }

            responseString +=
                "<div class=\"result\"><a href=\"" + result + "\">" + displayName + "</a></div>";
        }

        // Trailer
        responseString +=
            "    </article>\
</body>\
</html>";

        response.assign(responseString.begin(), responseString.end());

        return true;
    } else
        return serve(url, response);

    return false;
}
