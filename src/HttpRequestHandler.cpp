/**
 * @file HttpRequestHandler.cpp
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.5
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include "HttpRequestHandler.h"
#include "HttpResponses.h"

#include <unicode/uchar.h>
#include <unicode/ustring.h>

#include <chrono>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <regex>
#include <sstream>

using namespace std;

HttpRequestHandler::HttpRequestHandler(string homePath, bool imageMode) {
    this->homePath = homePath;
    this->imagemode = imageMode;

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
    this->vocabTableName = this->imagemode ? "images_vocab" : "webpage_vocab";
    const char* vocabFile = this->imagemode ? "images_vocab.db" : "index_vocab.db";

    // Pointer for read statement
    sqlite3_stmt* stmt;
    // Statement structure
    string sql = string("SELECT vocabulary FROM ") + vocabTableName;

    // Opens vocabulary database
    if (sqlite3_open(vocabFile, &database_vocab) != SQLITE_OK) {
        cerr << "Error opening Vocabulary (" << vocabFile << "): " << sqlite3_errmsg(database_vocab)
             << endl;
        database_vocab = nullptr;
        return false;
    } else {
        cout << "Vocabulary opened successfully: " << vocabFile << endl;
        cout << "Search mode: " << (this->imagemode ? "IMAGES" : "HTML") << endl;
    }

    // Compiles SQL statement
    if (sqlite3_prepare_v2(database_vocab, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(database_vocab) << endl;
        return false;
    }

    wstring_convert<codecvt_utf8<char32_t>, char32_t> converter;
    uint32_t words = 0;
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
                } else if (word.size() >= 5) {
                    // Inserts word into Trie once a non-word character is found
                    trie->insert(word);
                    word.clear();
                    words++;
                    if (words % 1000 == 0) {
                        cout << "Words inserted: " << words << endl;
                    }
                }
            }
            if (word.size() >= 5) {
                // Inserts last word if applicable
                trie->insert(word);
                word.clear();
                words++;
            }
        }
    }

    sqlite3_finalize(stmt);
    cout << "Total words inserted: " << words << endl;
    sqlite3_close(database_vocab);
    cout << "Vocabulary closed" << endl;
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
    cout << "Home absolute: " << homeAbsolutePath << endl;

    auto relativePath = homeAbsolutePath / url.substr(1);
    cout << "Relative path: " << relativePath << endl;

    string path = filesystem::absolute(relativePath.make_preferred()).string();
    cout << "Final path: " << path << endl;
    cout << "File exists: " << filesystem::exists(path) << endl;

    // Security check: prevent directory traversal
    if (path.substr(0, homeAbsolutePath.string().size()) != homeAbsolutePath) {
        cout << "SECURITY: Path outside home directory" << endl;
        return false;
    }

    // Check if file exists before trying to open
    if (!filesystem::exists(path)) {
        cout << "ERROR: File does not exist at: " << path << endl;
        return false;
    }

    // Check if it's a regular file
    if (!filesystem::is_regular_file(path)) {
        cout << "ERROR: Not a regular file: " << path << endl;
        return false;
    }

    // Serves file
    ifstream file(path, ios::binary);
    if (file.fail()) {
        cout << "ERROR: Failed to open file: " << path << endl;
        return false;
    }

    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    response.resize(fileSize);
    file.read(response.data(), fileSize);

    return true;
}

/**
 * @brief URL encodes a string (replaces spaces and special characters)
 *
 * @param str Input string
 * @return URL-encoded string
 */
string urlEncode(const string& str) {
    ostringstream encoded;
    encoded.fill('0');
    encoded << hex;

    for (unsigned char c : str) {
        // Keep alphanumeric and safe characters
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            encoded << c;
        }
        // Space becomes %20
        else if (c == ' ') {
            encoded << "%20";
        }
        // Other characters are percent-encoded
        else {
            encoded << uppercase;
            encoded << '%' << setw(2) << int((unsigned char)c);
            encoded << nouppercase;
        }
    }

    return encoded.str();
}

/**
 * @brief Cleans HTML content to extract readable text
 *
 * @param html Raw HTML content
 * @return Cleaned text without tags
 */
string cleanHtmlContent(const string& html) {
    string result = html;

    // Remove script tags and their content
    regex scriptRegex("<script[^>]*>.*?</script>", regex::icase);
    result = regex_replace(result, scriptRegex, " ");

    // Remove style tags and their content
    regex styleRegex("<style[^>]*>.*?</style>", regex::icase);
    result = regex_replace(result, styleRegex, " ");

    // Remove all HTML tags
    regex tagRegex("<[^>]*>");
    result = regex_replace(result, tagRegex, " ");

    // Replace multiple spaces with single space
    regex spaceRegex("\\s+");
    result = regex_replace(result, spaceRegex, " ");

    // Trim leading and trailing spaces
    size_t start = result.find_first_not_of(" \t\n\r");
    size_t end = result.find_last_not_of(" \t\n\r");

    if (start == string::npos || end == string::npos) {
        return "";
    }

    return result.substr(start, end - start + 1);
}

/**
 * @brief Generates a snippet from HTML file content
 *
 * @param filePath Path to the HTML file
 * @param maxWords Maximum number of words in snippet
 * @return Snippet text with ellipsis if truncated
 */
string generateSnippet(const string& filePath, int maxWords = 30) {
    ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }

    // Read entire file
    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    file.close();

    // Clean HTML content
    string cleanText = cleanHtmlContent(content);

    if (cleanText.empty()) {
        return "";
    }

    // Extract first N words
    istringstream iss(cleanText);
    string word;
    vector<string> words;

    while (iss >> word && words.size() < maxWords) {
        words.push_back(word);
    }

    if (words.empty()) {
        return "";
    }

    // Join words
    string snippet;
    for (size_t i = 0; i < words.size(); i++) {
        snippet += words[i];
        if (i < words.size() - 1) {
            snippet += " ";
        }
    }

    // Add ellipsis if there are more words
    bool hasMore = false;
    if (iss >> word) {
        hasMore = true;
    }

    if (hasMore || words.size() == maxWords) {
        snippet += "...";
    }

    return snippet;
}

/**
 * @brief Converts filename to title case
 *
 * @param filename Input filename
 * @return Cleaned and formatted title
 */
string cleanTitle(const string& filename) {
    string result = filename;

    // Replace underscores with spaces
    replace(result.begin(), result.end(), '_', ' ');

    // Remove parentheses and their content
    regex parenRegex("\\([^)]*\\)");
    result = regex_replace(result, parenRegex, "");

    // Trim spaces
    size_t start = result.find_first_not_of(" \t");
    size_t end = result.find_last_not_of(" \t");

    if (start != string::npos && end != string::npos) {
        result = result.substr(start, end - start + 1);
    }

    // Capitalize first letter of each word (simple title case)
    bool capitalizeNext = true;
    for (size_t i = 0; i < result.length(); i++) {
        if (capitalizeNext && isalpha(result[i])) {
            result[i] = toupper(result[i]);
            capitalizeNext = false;
        } else if (result[i] == ' ') {
            capitalizeNext = true;
        }
    }

    return result;
}

/**
 * @brief Cleans URL for display (removes leading slash)
 *
 * @param url Input URL
 * @return Cleaned URL
 */
string cleanUrl(const string& url) {
    if (url.empty()) {
        return url;
    }

    // Remove leading slash
    if (url[0] == '/') {
        return url.substr(1);
    }

    return url;
}

bool HttpRequestHandler::luckyHandler(vector<char>& response) {
    cout << "Lucky search request received" << endl;

    if (!database) {
        string jsonResponse = "{\"success\": false, \"error\": \"Database not available\"}";
        response.assign(jsonResponse.begin(), jsonResponse.end());
        return true;
    }

    // Fast method: uses rowid for efficient random selection
    sqlite3_stmt* stmt;
    string sql = string("SELECT path FROM ") + tableName +
                 " WHERE rowid >= (ABS(RANDOM()) % (SELECT MAX(rowid) FROM " + tableName +
                 ")) LIMIT 1;";

    if (sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        cerr << "Failed to prepare lucky statement: " << sqlite3_errmsg(database) << endl;
        string jsonResponse = "{\"success\": false, \"error\": \"Query failed\"}";
        response.assign(jsonResponse.begin(), jsonResponse.end());
        return true;
    }

    string randomPath = "";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* path = (const char*)sqlite3_column_text(stmt, 0);
        if (path) {
            randomPath = string(path);
        }
    }

    sqlite3_finalize(stmt);

    string jsonResponse;
    if (!randomPath.empty()) {
        cout << "Lucky search found: " << randomPath << endl;
        jsonResponse = "{\"success\": true, \"path\": \"" + randomPath + "\"}";
    } else {
        cout << "Lucky search found no results" << endl;
        jsonResponse = "{\"success\": false, \"error\": \"No entries found\"}";
    }

    response.assign(jsonResponse.begin(), jsonResponse.end());
    return true;
}

bool HttpRequestHandler::predictHandler(std::vector<char>& response, HttpArguments& arguments) {
    cout << "Predict request received" << endl;
    string query;
    if (arguments.find("q") != arguments.end())
        query = arguments["q"];
    cout << "Query: " << query << endl;

    // Clear previous suggestions and collect new ones
    trie->collectWords.clear();
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

bool HttpRequestHandler::homePageHandler(std::vector<char>& response) {
    // Serves the home page with autocomplete functionality
	string responseString = Responses::homePageResponse();

    response.assign(responseString.begin(), responseString.end());
    return true;
}

bool HttpRequestHandler::imageHandler(std::vector<char>& response,
                                      HttpArguments& arguments,
                                      std::string& url) {
    // Check if there's a "view" parameter to show the viewer page
    if (arguments.find("view") != arguments.end()) {
        // Extract filename from URL (remove query parameters)
        string cleanUrlStr = url;
        size_t queryPos = cleanUrlStr.find('?');
        if (queryPos != string::npos) {
            cleanUrlStr = cleanUrlStr.substr(0, queryPos);
        }

        size_t lastSlash = cleanUrlStr.find_last_of('/');
        string filename =
            (lastSlash != string::npos) ? cleanUrlStr.substr(lastSlash + 1) : cleanUrlStr;

        // Extract title from filename (remove extension)
        string title = filename;
        size_t lastDot = title.find_last_of('.');
        if (lastDot != string::npos) {
            title = title.substr(0, lastDot);
        }

        // Clean title for display
        string cleanedTitle = cleanTitle(title);

        // URL-encode the clean URL for the image src
        string encodedImageUrl = urlEncode(cleanUrlStr);

        // Build image viewer page
		string responseString = Responses::imagePageResponse(
			cleanedTitle, encodedImageUrl, filename, cleanUrlStr);

        response.assign(responseString.begin(), responseString.end());
        return true;
    }
    return serve(url, response);
}

bool HttpRequestHandler::searchHandler(std::vector<char>& response, HttpArguments& arguments) {
    string searchString;
    if (arguments.find("q") != arguments.end())
        searchString = arguments["q"];

    // HTML Header with autocomplete and enhanced styling
	string responseString = Responses::searchPageStart(searchString);

    // Start timer
    auto startTime = chrono::high_resolution_clock::now();
    float searchTime = 0.0F;
    vector<pair<string, string>> results;

    if (!searchString.empty() && database) {
        sqlite3_stmt* stmt;
        string sql = string("SELECT path, snippet, BM25(") + tableName + ") AS rank " + "FROM " +
                     tableName + " WHERE " + tableName + " MATCH ? ORDER BY rank ASC LIMIT 100;";

        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, searchString.c_str(), -1, SQLITE_TRANSIENT);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* path = (const char*)sqlite3_column_text(stmt, 0);
                const char* snippet = (const char*)sqlite3_column_text(stmt, 1);

                if (path) {
                    string snippetStr = snippet ? string(snippet) : "";
                    results.push_back(make_pair(string(path), snippetStr));
                }
            }

            sqlite3_finalize(stmt);
        }
    }

    // Stop timer
    auto endTime = chrono::high_resolution_clock::now();
    searchTime = chrono::duration<float>(endTime - startTime).count();

    // Print search results count
    responseString += "<div class=\"results-stats\">" + to_string(results.size()) +
                      " resultados (" + to_string(searchTime) + " segundos)</div>";

    responseString += "<div class=\"results\">";

    for (auto& result : results) {
        string path = result.first;
        string precomputedSnippet = result.second;

        // Extract display name from path
        size_t lastSlash = path.find_last_of('/');
        string displayName = (lastSlash != string::npos) ? path.substr(lastSlash + 1) : path;

        // Remove extension
        size_t lastDot = displayName.find_last_of('.');
        if (lastDot != string::npos) {
            displayName = displayName.substr(0, lastDot);
        }

        // Clean title for display
        string cleanedTitle = cleanTitle(displayName);

        // Use precomputed snippet if available
        string snippet;
        if (!precomputedSnippet.empty()) {
            snippet = precomputedSnippet;
        } else {
            filesystem::path fullPath = filesystem::path(homePath) / path.substr(1);
            snippet = generateSnippet(fullPath.string());
            if (snippet.empty()) {
                snippet = "Información sobre " + cleanedTitle + ".";
            }
        }

        // Clean URL for display
        string displayUrl = cleanUrl(path);

        // Build result HTML based on mode
        if (imagemode) {
            // IMAGE MODE
            string encodedPath = urlEncode(path);

            responseString += "<div class=\"result image-result\">";
            responseString += "<div class=\"image-thumbnail\">";
            responseString += "<a href=\"" + path + "?view=1\"><img src=\"" + encodedPath +
                              "\" alt=\"" + cleanedTitle + "\"></a>";
            responseString += "</div>";
            responseString += "<div class=\"image-details\">";
            responseString += "<div class=\"url\">" + displayUrl + "</div>";
            responseString +=
                "<a class=\"title\" href=\"" + path + "?view=1\">" + cleanedTitle + "</a>";
            responseString += "<div class=\"snippet\">" + snippet + "</div>";
            responseString += "</div>";
            responseString += "</div>";
        } else {
            // HTML MODE
            responseString += "<div class=\"result\">";
            responseString += "<div class=\"url\">" + displayUrl + "</div>";
            responseString += "<a class=\"title\" href=\"" + path + "\">" + cleanedTitle + "</a>";
            responseString += "<div class=\"snippet\">" + snippet + "</div>";
            responseString += "</div>";
        }
    }

    responseString += Responses::searchPageEnd();

    response.assign(responseString.begin(), responseString.end());
    return true;
}

bool HttpRequestHandler::handleRequest(string url,
                                       HttpArguments arguments,
                                       vector<char>& response) {
    //=============== LUCKY HANDLER (FAST VERSION) ===============//
    string luckyPage = "/lucky";
    if (url == luckyPage) {
        return HttpRequestHandler::luckyHandler(response);
    }

    //=============== PREDICT HANDLER ===============//
    string predictPage = "/predict";
    if (url.substr(0, predictPage.size()) == predictPage) {
        return predictHandler(response, arguments);
    }

    //=============== HOME PAGE HANDLER ===============//
    string homePage = "/";
    if (url == homePage) {
        return homePageHandler(response);
    }

    //=============== IMAGE VIEWER HANDLER ===============//
    string specialPage = "/special/";
    if (url.substr(0, specialPage.size()) == specialPage &&
        (url.find(".png") != string::npos || url.find(".jpg") != string::npos ||
         url.find(".jpeg") != string::npos || url.find(".PNG") != string::npos ||
         url.find(".JPG") != string::npos || url.find(".JPEG") != string::npos)) {
        return imageHandler(response, arguments, url);
    }

    //=============== SEARCH HANDLER ===============//
    string searchPage = "/search";
    if (url.substr(0, searchPage.size()) == searchPage) {
        return searchHandler(response, arguments);
    } else
        return serve(url, response);

    return false;
}
