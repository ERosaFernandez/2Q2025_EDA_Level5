/**
 * @file HttpRequestHandler.cpp
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.5
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
    string responseString = string(
        "<!DOCTYPE html>\
<html>\
<head>\
    <meta charset=\"utf-8\" />\
    <title>EDAoogle</title>\
    <link rel=\"preload\" href=\"https://fonts.googleapis.com\" />\
    <link rel=\"preload\" href=\"https://fonts.gstatic.com\" crossorigin />\
    <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;600&display=swap\" rel=\"stylesheet\" />\
    <link rel=\"preload\" href=\"css/style.css\" />\
    <link rel=\"stylesheet\" href=\"css/style.css\" />\
    <style>\
        .search {\
            position: relative;\
        }\
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
            width: min(90vw, 500px);\
            top: 100%;\
            left: 0;\
            margin-top: 4px;\
        }\
        .suggestion-item {\
            padding: 10px 15px;\
            cursor: pointer;\
            border-bottom: 1px solid #f0f0f0;\
        }\
        .suggestion-item:last-child {\
            border-bottom: none;\
        }\
        .suggestion-item:hover {\
            background-color: #f5f5f5;\
        }\
    </style>\
    <script>\
        document.addEventListener('DOMContentLoaded', () => {\
            const input = document.querySelector('input[name=\"q\"]');\
            const form = document.querySelector('form');\
            const searchContainer = document.querySelector('.search');\
            let suggestionsDiv = document.createElement('div');\
            suggestionsDiv.id = 'suggestions';\
            searchContainer.appendChild(suggestionsDiv);\
            \
            let debounceTimer;\
            \
            input.addEventListener('input', (e) => {\
                clearTimeout(debounceTimer);\
                \
                const fullQuery = e.target.value;\
                const words = fullQuery.split(' ');\
                const lastWord = words[words.length - 1].trim();\
                \
                if (lastWord.length < 2) {\
                    suggestionsDiv.style.display = 'none';\
                    return;\
                }\
                \
                debounceTimer = setTimeout(async () => {\
                    try {\
                        const response = await fetch('/predict?q=' + encodeURIComponent(lastWord));\
                        const suggestions = await response.json();\
                        \
                        if (suggestions.length > 0) {\
                            suggestionsDiv.innerHTML = '';\
                            suggestions.forEach(suggestion => {\
                                const div = document.createElement('div');\
                                div.className = 'suggestion-item';\
                                div.textContent = suggestion;\
                                div.onclick = () => {\
                                    words[words.length - 1] = suggestion;\
                                    input.value = words.join(' ') + ' ';\
                                    suggestionsDiv.style.display = 'none';\
                                    input.focus();\
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
                if (e.target !== input && !suggestionsDiv.contains(e.target)) {\
                    suggestionsDiv.style.display = 'none';\
                }\
            });\
        });\
    </script>\
</head>\
\
<body>\
    <article class=\"edaoogle\">\
        <div class=\"title\">EDAoogle</div>\
        <div class=\"search\">\
            <form action=\"/search\" method=\"get\">\
                <input type=\"text\" name=\"q\" autofocus>\
                <button type=\"submit\" class=\"search-btn\">Buscar</button>\
            </form>\
        </div>\
    </article>\
</body>\
</html>");

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
        string responseString =
            string(
                "<!DOCTYPE html>\
<html>\
<head>\
    <meta charset=\"utf-8\" />\
    <title>") +
            cleanedTitle +
            string(
                " - EDAoogle</title>\
    <link rel=\"preload\" href=\"https://fonts.googleapis.com\" />\
    <link rel=\"preload\" href=\"https://fonts.gstatic.com\" crossorigin />\
    <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;600&display=swap\" rel=\"stylesheet\" />\
    <link rel=\"stylesheet\" href=\"../css/style.css\" />\
    <style>\
        .image-viewer {\
            max-width: 1200px;\
            margin: 2rem auto;\
            padding: 0 2rem;\
        }\
        .image-header {\
            margin-bottom: 2rem;\
        }\
        .back-link {\
            color: #1a0dab;\
            text-decoration: none;\
            font-size: 0.9rem;\
            display: inline-block;\
            margin-bottom: 1rem;\
        }\
        .back-link:hover {\
            text-decoration: underline;\
        }\
        .image-title {\
            font-size: 2rem;\
            font-weight: 600;\
            color: #202124;\
            margin-bottom: 0.5rem;\
        }\
        .image-filename {\
            font-size: 0.9rem;\
            color: #70757a;\
        }\
        .image-container {\
            background: #f8f9fa;\
            border-radius: 8px;\
            padding: 2rem;\
            text-align: center;\
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);\
        }\
        .image-container img {\
            max-width: 100%;\
            height: auto;\
            border-radius: 4px;\
            box-shadow: 0 2px 8px rgba(0,0,0,0.15);\
        }\
        .image-info {\
            margin-top: 1.5rem;\
            text-align: left;\
            background: white;\
            padding: 1.5rem;\
            border-radius: 8px;\
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);\
        }\
        .info-row {\
            margin-bottom: 0.75rem;\
        }\
        .info-label {\
            font-weight: 600;\
            color: #202124;\
        }\
        .info-value {\
            color: #5f6368;\
        }\
    </style>\
</head>\
<body>\
    <div class=\"image-viewer\">\
        <div class=\"image-header\">\
            <a href=\"/\" class=\"back-link\">← Volver a EDAoogle</a>\
            <h1 class=\"image-title\">") +
            cleanedTitle +
            string(
                "</h1>\
            <div class=\"image-filename\">") +
            filename +
            string(
                "</div>\
        </div>\
        <div class=\"image-container\">\
            <img src=\"") +
            encodedImageUrl + string("\" alt=\"") + cleanedTitle +
            string(
                "\" onerror=\"this.onerror=null; this.src=''; this.alt='Error: Imagen no encontrada';\" />\
        </div>\
        <div class=\"image-info\">\
            <div class=\"info-row\">\
                <span class=\"info-label\">Nombre del archivo:</span> \
                <span class=\"info-value\">") +
            filename +
            string(
                "</span>\
            </div>\
            <div class=\"info-row\">\
                <span class=\"info-label\">Ubicación:</span> \
                <span class=\"info-value\">") +
            cleanUrlStr +
            string(
                "</span>\
            </div>\
        </div>\
    </div>\
</body>\
</html>");

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
    string responseString = R"(<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" + searchString +
                            R"( - EDAoogle</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Product+Sans:wght@400;500;700&display=swap" rel="stylesheet">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        :root {
            --google-blue: #4285f4;
            --google-red: #ea4335;
            --google-green: #34a853;
            --purple-accent: #a142f4;
            --bg-light: #ffffff;
            --bg-dark: #202124;
            --text-light: #202124;
            --text-dark: #e8eaed;
            --input-light: #ffffff;
            --input-dark: #303134;
            --shadow-light: rgba(0, 0, 0, 0.1);
            --shadow-dark: rgba(0, 0, 0, 0.3);
        }

        @media (prefers-color-scheme: dark) {
            :root {
                --bg: var(--bg-dark);
                --text: var(--text-dark);
                --input-bg: var(--input-dark);
                --shadow: var(--shadow-dark);
            }
        }

        @media (prefers-color-scheme: light) {
            :root {
                --bg: var(--bg-light);
                --text: var(--text-light);
                --input-bg: var(--input-light);
                --shadow: var(--shadow-light);
            }
        }

        body {
            font-family: 'Product Sans', 'Roboto', 'Arial', sans-serif;
            background: var(--bg);
            color: var(--text);
            min-height: 100vh;
            transition: background 0.3s ease, color 0.3s ease;
        }

        header {
            position: sticky;
            top: 0;
            background: var(--bg);
            border-bottom: 1px solid rgba(128, 128, 128, 0.1);
            padding: 16px 24px;
            display: flex;
            align-items: center;
            gap: 32px;
            z-index: 100;
            box-shadow: 0 1px 6px var(--shadow);
            backdrop-filter: blur(10px);
            animation: slideDown 0.4s ease-out;
        }

        @keyframes slideDown {
            from {
                transform: translateY(-100%);
                opacity: 0;
            }
            to {
                transform: translateY(0);
                opacity: 1;
            }
        }

        .logo-small {
            font-size: 24px;
            font-weight: 400;
            text-decoration: none;
            letter-spacing: -1px;
            transition: transform 0.3s ease;
            white-space: nowrap;
        }

        .logo-small:hover {
            transform: scale(1.05);
        }

        .logo-small span:nth-child(1) { color: var(--google-blue); }
        .logo-small span:nth-child(2) { color: var(--google-red); }
        .logo-small span:nth-child(3) { color: #fbbc04; }
        .logo-small span:nth-child(4) { color: var(--google-blue); }
        .logo-small span:nth-child(5) { color: var(--google-green); }
        .logo-small span:nth-child(6) { color: var(--google-red); }
        .logo-small span:nth-child(7) { color: var(--purple-accent); }
        .logo-small span:nth-child(8) { color: #24c1e0; }

        .search-header {
            flex: 1;
            max-width: 600px;
            position: relative;
        }

        .search-container {
            position: relative;
            background: var(--input-bg);
            border-radius: 24px;
            box-shadow: 0 1px 6px var(--shadow);
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        }

        .search-container:focus-within {
            box-shadow: 0 4px 20px rgba(66, 133, 244, 0.3);
        }

        .search-input-wrapper {
            display: flex;
            align-items: center;
            padding: 0 16px;
        }

        .search-icon {
            color: #9aa0a6;
            margin-right: 12px;
            font-size: 18px;
        }

        input[type="text"] {
            flex: 1;
            border: none;
            outline: none;
            padding: 10px 0;
            font-size: 16px;
            background: transparent;
            color: var(--text);
            font-family: inherit;
        }

        button[type="submit"] {
            padding: 8px 16px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-family: inherit;
            font-weight: 500;
            cursor: pointer;
            background: var(--google-blue);
            color: white;
            transition: all 0.3s ease;
            margin-left: 12px;
        }

        button[type="submit"]:hover {
            background: #1a73e8;
            transform: scale(1.05);
        }

        #suggestions {
            position: absolute;
            top: calc(100% + 8px);
            left: 0;
            right: 0;
            background: var(--input-bg);
            border-radius: 16px;
            box-shadow: 0 8px 32px var(--shadow);
            max-height: 400px;
            overflow-y: auto;
            display: none;
            z-index: 10001;
        }

        #suggestions.show {
            display: block;
            animation: dropdownAppear 0.3s ease;
        }

        @keyframes dropdownAppear {
            from {
                opacity: 0;
                transform: translateY(-10px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        #suggestions-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: transparent;
            z-index: 10000;
            display: none;
        }

        #suggestions-overlay.show {
            display: block;
        }

        .suggestion-item {
            padding: 12px 16px;
            cursor: pointer;
            transition: all 0.2s ease;
            color: var(--text);
            border-bottom: 1px solid rgba(128, 128, 128, 0.1);
        }

        .suggestion-item:last-child {
            border-bottom: none;
        }

        .suggestion-item::before {
            content: '🔍';
            margin-right: 12px;
            font-size: 16px;
        }

        .suggestion-item:hover {
            background: rgba(66, 133, 244, 0.08);
            padding-left: 20px;
        }

        main {
            max-width: 800px;
            margin: 0 auto;
            padding: 24px;
        }

        .results-stats {
            margin-bottom: 24px;
            font-size: 14px;
            color: #70757a;
            animation: fadeIn 0.5s ease;
        }

        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        .results {
            display: flex;
            flex-direction: column;
            gap: 32px;
        }

        .result {
            opacity: 0;
            animation: resultAppear 0.5s ease forwards;
            transition: transform 0.3s ease;
        }

        .result:hover {
            transform: translateX(8px);
        }

        @keyframes resultAppear {
            from {
                opacity: 0;
                transform: translateY(20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        .result:nth-child(1) { animation-delay: 0.1s; }
        .result:nth-child(2) { animation-delay: 0.15s; }
        .result:nth-child(3) { animation-delay: 0.2s; }
        .result:nth-child(4) { animation-delay: 0.25s; }
        .result:nth-child(5) { animation-delay: 0.3s; }
        .result:nth-child(6) { animation-delay: 0.35s; }
        .result:nth-child(7) { animation-delay: 0.4s; }
        .result:nth-child(8) { animation-delay: 0.45s; }
        .result:nth-child(9) { animation-delay: 0.5s; }
        .result:nth-child(10) { animation-delay: 0.55s; }

        .url {
            font-size: 14px;
            color: #006621;
            margin-bottom: 4px;
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .url::before {
            content: '🌐';
            font-size: 16px;
        }

        .title {
            display: inline-block;
            font-size: 20px;
            color: var(--google-blue);
            text-decoration: none;
            margin-bottom: 4px;
            transition: color 0.2s ease;
            font-weight: 400;
        }

        .title:hover {
            text-decoration: underline;
            color: #1a73e8;
        }

        .snippet {
            font-size: 14px;
            color: #4d5156;
            line-height: 1.6;
        }

        .image-result {
            display: flex;
            gap: 16px;
            align-items: flex-start;
        }

        .image-thumbnail {
            flex-shrink: 0;
            border-radius: 12px;
            overflow: hidden;
            box-shadow: 0 2px 8px var(--shadow);
            transition: all 0.3s ease;
        }

        .image-thumbnail:hover {
            transform: scale(1.05);
            box-shadow: 0 4px 16px var(--shadow);
        }

        .image-thumbnail img {
            width: 200px;
            height: 200px;
            object-fit: cover;
            display: block;
        }

        .image-details {
            flex: 1;
        }

        @media (max-width: 768px) {
            header {
                flex-direction: column;
                gap: 16px;
                padding: 12px 16px;
            }

            .search-header {
                width: 100%;
            }

            main {
                padding: 16px;
            }

            .image-result {
                flex-direction: column;
            }

            .image-thumbnail img {
                width: 100%;
                height: auto;
            }
        }

        ::-webkit-scrollbar {
            width: 8px;
        }

        ::-webkit-scrollbar-track {
            background: transparent;
        }

        ::-webkit-scrollbar-thumb {
            background: rgba(128, 128, 128, 0.3);
            border-radius: 4px;
        }

        ::-webkit-scrollbar-thumb:hover {
            background: rgba(128, 128, 128, 0.5);
        }
    </style>
</head>
<body>
    <div id="suggestions-overlay"></div>

    <header>
        <a href="/" class="logo-small">
            <span>E</span><span>D</span><span>A</span><span>o</span><span>o</span><span>g</span><span>l</span><span>e</span>
        </a>

        <div class="search-header">
            <form action="/search" method="get">
                <div class="search-container">
                    <div class="search-input-wrapper">
                        <span class="search-icon">🔍</span>
                        <input type="text" name="q" value=")" +
                            searchString + R"(" autocomplete="off" autofocus>
                        <button type="submit">Buscar</button>
                    </div>
                </div>
                <div id="suggestions"></div>
            </form>
        </div>
    </header>

    <main>)";

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

    responseString += R"(
        </div>
    </main>

    <script>
        const input = document.querySelector('input[name="q"]');
        const suggestionsDiv = document.getElementById('suggestions');
        const suggestionsOverlay = document.getElementById('suggestions-overlay');
        let debounceTimer;

        input.addEventListener('input', (e) => {
            clearTimeout(debounceTimer);
            const fullQuery = e.target.value;
            const words = fullQuery.split(' ');
            const lastWord = words[words.length - 1].trim();

            if (lastWord.length < 2) {
                suggestionsDiv.classList.remove('show');
                suggestionsOverlay.classList.remove('show');
                return;
            }

            debounceTimer = setTimeout(async () => {
                try {
                    const response = await fetch('/predict?q=' + encodeURIComponent(lastWord));
                    const suggestions = await response.json();

                    if (suggestions.length > 0) {
                        suggestionsDiv.innerHTML = '';
                        suggestions.forEach(suggestion => {
                            const div = document.createElement('div');
                            div.className = 'suggestion-item';
                            div.textContent = suggestion;
                            div.onclick = () => {
                                words[words.length - 1] = suggestion;
                                input.value = words.join(' ') + ' ';
                                suggestionsDiv.classList.remove('show');
                                suggestionsOverlay.classList.remove('show');
                                input.focus();
                            };
                            suggestionsDiv.appendChild(div);
                        });
                        suggestionsDiv.classList.add('show');
                        suggestionsOverlay.classList.add('show');
                    } else {
                        suggestionsDiv.classList.remove('show');
                        suggestionsOverlay.classList.remove('show');
                    }
                } catch (error) {
                    console.error('Error:', error);
                }
            }, 300);
        });

        document.addEventListener('click', (e) => {
            if (e.target !== input && !suggestionsDiv.contains(e.target)) {
                suggestionsDiv.classList.remove('show');
                suggestionsOverlay.classList.remove('show');
            }
        });

        suggestionsOverlay.addEventListener('click', () => {
            suggestionsDiv.classList.remove('show');
            suggestionsOverlay.classList.remove('show');
        });

        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                suggestionsDiv.classList.remove('show');
                suggestionsOverlay.classList.remove('show');
            }
        });

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    entry.target.style.opacity = '1';
                    entry.target.style.transform = 'translateY(0)';
                }
            });
        }, { threshold: 0.1 });

        document.querySelectorAll('.result').forEach(result => {
            observer.observe(result);
        });
    </script>
</body>
</html>
)";

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
