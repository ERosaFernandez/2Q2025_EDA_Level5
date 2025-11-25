/**
 * @file mkindex.cpp
 * @author Marc S. Ressl
 * @brief Makes a database index
 * @version 1.0
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <sqlite3.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>

#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "CommandLineParser.h"

using namespace std;

/**
 * @name helpMessage
 * @brief Sends a message through terminal for guidance
 */
bool helpMessage() {
    cout << "/==========================================================================/" << endl
         << "Parameters:" << endl
         << "-mode (image / html): mandatory," << endl
         << "defines which and how the files are indexed." << endl
         << "-append (index / vocab / both): optional," << endl
         << "defines whether to remove or add new entries to the old databases." << endl
         << "-skipvocab (no argument): optional," << endl
         << "specifies whether to skip or not the vocabulary generation for the database." << endl
         << "-path (insertYourFolderRelativePath): mandatory," << endl
         << "specifies relative path for files to be indexed." << endl
         << endl;

    cout << "example for Linux:" << endl
         << "./mkindex -mode image -skipvocab -path ../www/special/" << endl
         << "example for Windows:" << endl
         << "mkindex.exe -mode html -append both -path ../../../../www" << endl
         << "example for macOS: install Linux" << endl
         << "/==========================================================================/" << endl;

    return 0;
}

static int onDatabaseEntry(void* userdata, int argc, char** argv, char** azColName) {
    cout << "--- Entry" << endl;
    for (int i = 0; i < argc; i++) {
        if (argv[i])
            cout << azColName[i] << ": " << argv[i] << endl;
        else
            cout << azColName[i] << ": " << "NULL" << endl;
    }

    return 0;
}

string removeHTMLTags(const string& html) {
    string result;
    string normalized;

    result.reserve(html.size());

    bool insideTag = false;
    bool lastWasSpace = false;

    for (size_t i = 0; i < html.size(); i++) {
        if (html[i] == '<') {
            insideTag = true;

            // Detects and skips <script>...</script>
            if (i + 7 <= html.size() && html.substr(i, 7) == "<script") {
                // Looks for </script>
                size_t scriptEnd = html.find("</script>", i);
                if (scriptEnd != string::npos) {
                    i = scriptEnd + 8;  // Skips ahead of </script>
                    insideTag = false;
                    continue;
                }
            }

            // Detects and skips <style>...</style>
            if (i + 6 <= html.size() && html.substr(i, 6) == "<style") {
                // Buscar </style>
                size_t styleEnd = html.find("</style>", i);
                if (styleEnd != string::npos) {
                    i = styleEnd + 7;  // Skips ahead of </style>
                    insideTag = false;
                    continue;
                }
            }

        } else if (html[i] == '>') {
            insideTag = false;
        } else if (!insideTag) {
            result += html[i];
        }
    }

    // Reduces line breaks and multiple spaces
    for (unsigned char c : result) {
        if (isspace(c)) {
            if (!lastWasSpace) {
                normalized += ' ';
                lastWasSpace = true;
            }
        } else {
            normalized += c;
            lastWasSpace = false;
        }
    }

    return normalized;
}

/**
 * @brief Generates a snippet from cleaned text content
 *
 * @param cleanText Already cleaned text (no HTML tags)
 * @param maxWords Maximum number of words in snippet
 * @return Snippet text with ellipsis if truncated
 */
string generateSnippetFromCleanText(const string& cleanText, int maxWords = 100) {
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

size_t vocabulary(const string& cleanContent,
                  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>& converter,
                  set<string>& vocabSet) {
    u32string convertedString = converter.from_bytes(cleanContent);
    u32string word;
    word.reserve(20);

    for (UChar32 c : convertedString) {
        if (u_isalpha(c)) {
            // Builds word character by character
            word.push_back(u_tolower(c));
        } else if (word.size() >= 5) {
            // Inserts word into vocabulary once a non-word character is found
            vocabSet.insert(converter.to_bytes(word));
            word.clear();
        } else
            word.clear();
    }
    // Adds last word if applicable
    if (word.size() >= 5) {
        vocabSet.insert(converter.to_bytes(word));
        word.clear();
    }
    return vocabSet.size();
}

bool setupDatabase(const char* databaseFile,
                   sqlite3*& database,
                   const char* tableName,
                   char*& databaseErrorMessage,
                   bool append,
                   bool vocabulary,
                   sqlite3_stmt*& stmt) {
    cout << "Starting Indexing..." << endl;

    // Open database file
    cout << "Opening database..." << endl;
    if (sqlite3_open(databaseFile, &database) != SQLITE_OK) {
        cout << "Can't open database: " << sqlite3_errmsg(database) << endl;

        return 1;
    }

    // Additional settings and extensions

    if (sqlite3_exec(database, "PRAGMA secure_delete = OFF;", nullptr, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "error with PRAGMA setting: " << sqlite3_errmsg(database) << endl;
        return 1;
    }
    if (sqlite3_exec(
            database, "PRAGMA locking_mode = EXCLUSIVE;", nullptr, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "error with PRAGMA setting: " << sqlite3_errmsg(database) << endl;
        return 1;
    }
    if (sqlite3_exec(database, "PRAGMA cache_size = -524288;", nullptr, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "error with PRAGMA setting: " << sqlite3_errmsg(database) << endl;
        return 1;
    }
    if (sqlite3_exec(database, "PRAGMA temp_store = MEMORY;", nullptr, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "error with PRAGMA setting: " << sqlite3_errmsg(database) << endl;
        return 1;
    }
    if (sqlite3_exec(
            database, "PRAGMA mmap_size = 1073741824;", nullptr, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "error with PRAGMA setting: " << sqlite3_errmsg(database) << endl;
        return 1;
    }
    if (sqlite3_exec(database, "PRAGMA journal_mode = WAL;", nullptr, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "error with PRAGMA setting: " << sqlite3_errmsg(database) << endl;
        return 1;
    }
    if (sqlite3_exec(database, "PRAGMA synchronous = OFF;", nullptr, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "error with PRAGMA setting: " << sqlite3_errmsg(database) << endl;
        return 1;
    }
    cout << "Succesfully loaded custom settings " << endl;

    // Create FTS5 virtual table
    cout << "Creating FTS5 virtual table: " << tableName << "..." << endl;

    // Drop old table if not appending
    if (!append) {
        string dropTableSQL = string("DROP TABLE IF EXISTS ") + tableName + ";";
        if (sqlite3_exec(database, dropTableSQL.c_str(), NULL, 0, &databaseErrorMessage) ==
            SQLITE_OK) {
            cout << "Dropping table" << endl;
            // Continue anyway - table might not exist
        }
    }

    string createTableSQL;

    if (!vocabulary) {
        createTableSQL = string("CREATE VIRTUAL TABLE IF NOT EXISTS ") + tableName +
                         " USING fts5("
                         "path UNINDEXED,"
                         "title,"
                         "content,"
                         "snippet UNINDEXED,"
                         "detail = none,"
                         "tokenize = 'unicode61 remove_diacritics 2');";
    } else {
        createTableSQL = string("CREATE VIRTUAL TABLE IF NOT EXISTS ") + tableName +
                         " USING fts5("
                         "vocabulary);";
    }

    if (sqlite3_exec(database, createTableSQL.c_str(), NULL, 0, &databaseErrorMessage) !=
        SQLITE_OK) {
        cout << "Error: " << sqlite3_errmsg(database) << endl;
        sqlite3_close(database);
        return 1;
    }

    // Begin transaction
    cout << "Starting transaction..." << endl;
    sqlite3_exec(database, "BEGIN TRANSACTION;", NULL, 0, NULL);

    // Prepare SQL statement
    cout << "Preparing SQL statement..." << endl;
    string insertSQL;

    if (!vocabulary) {
        insertSQL = string("INSERT INTO ") + tableName +
                    " (path, title, content, snippet) VALUES (?, ?, ?, ?);";
    } else {
        insertSQL = string("INSERT INTO ") + tableName + " (vocabulary) VALUES (?);";
    }

    if (sqlite3_prepare_v2(database, insertSQL.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        cout << "Error preparing statement: " << sqlite3_errmsg(database) << endl;
        sqlite3_close(database);
        return 1;
    }
    return 0;
}

bool finalizeDatabase(sqlite3_stmt*& stmt,
                      sqlite3*& database,
                      char*& databaseErrorMessage,
                      const char* databaseFile,
                      int processedFiles,
                      const char* tableName) {
    // Clears statement
    sqlite3_finalize(stmt);

    // Commits transaction
    cout << "Committing transaction..." << endl;
    if (sqlite3_exec(database, "COMMIT;", NULL, 0, &databaseErrorMessage) != SQLITE_OK) {
        cout << "Error committing transaction: " << sqlite3_errmsg(database) << endl;
        return 1;
    } else if (processedFiles == -1) {
        // Special case for vocabulary
        cout << "Successfully implemented vocabulary file" << databaseFile << endl;
    } else {
        cout << "Successfully indexed " << processedFiles << " files." << endl;

        // Optimizes index table
        string optimizeString =
            string("INSERT INTO ") + tableName + " (" + tableName + ") VALUES ('optimize');";

        if (sqlite3_exec(database, optimizeString.c_str(), NULL, 0, &databaseErrorMessage) !=
            SQLITE_OK) {
            cout << "Error optimizing: " << sqlite3_errmsg(database) << endl;
            return 1;
        }
        cout << "Successfully optimized index " << endl;
    }

    // Close database
    cout << "Closing database..." << endl;
    sqlite3_close(database);

    return 0;
}

bool indexDatabase(const string& inputFolder,
                   const char* databaseFile,
                   set<string>& vocabSet,
                   bool append) {
    // Set up variables
    sqlite3* database;
    char* databaseErrorMessage = nullptr;
    const char* tableName = "webpage_index";
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    int processedFiles = 0;
    sqlite3_stmt* stmt;

    if (setupDatabase(databaseFile, database, tableName, databaseErrorMessage, append, 0, stmt) !=
        0) {
        return 1;
    }

    // Iterate through files in the specified folder
    cout << "Indexing HTML files from folder: " << inputFolder << endl;

    for (const auto& entry : filesystem::recursive_directory_iterator(inputFolder)) {
        // Only process .html files
        if (entry.is_regular_file() && entry.path().extension() == ".html") {
            cout << "Processing: " << entry.path().filename().string() << endl;

            // Reads file content
            ifstream fileStream(entry.path());
            if (fileStream.fail()) {
                cout << "  Error opening file, skipping..." << endl;
                continue;
            }

            string htmlContent((istreambuf_iterator<char>(fileStream)),
                               istreambuf_iterator<char>());
            fileStream.close();

            // Extract title
            string title = "No Title";
            size_t titleStart = htmlContent.find("<title>");
            size_t titleEnd = htmlContent.find("</title>");

            if (titleStart != string::npos && titleEnd != string::npos && titleEnd > titleStart) {
                title = htmlContent.substr(titleStart + 7, titleEnd - (titleStart + 7));
            }

            // Parse HTML content to plain text
            string cleanContent = removeHTMLTags(htmlContent);

            // Generate snippet from clean content (first 25 words)
            string snippet = generateSnippetFromCleanText(cleanContent, 60);

            // Sets relative path
            string relativePath = "/wiki/" + entry.path().filename().string();

            // Generates vocabulary
            cout << "  Successfully extracted vocabulary. "
                 << "Vocabulary size: " << vocabulary(cleanContent, converter, vocabSet) << endl;
            cout << "  Generated snippet: " << snippet.substr(0, 50) << "..." << endl;

            // Bind values to the prepared statement
            sqlite3_bind_text(stmt, 1, relativePath.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, cleanContent.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, snippet.c_str(), -1, SQLITE_TRANSIENT);

            // Executes statement
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                cout << "  Error inserting: " << sqlite3_errmsg(database) << endl;
            }

            // Resets for next iteration
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);

            processedFiles++;
        }
    }

    return finalizeDatabase(
        stmt, database, databaseErrorMessage, databaseFile, processedFiles, tableName);
}

bool imageDatabase(const string& inputFolder,
                   const char* databaseFile,
                   set<string>& vocabSet,
                   bool append) {
    // Set up variables
    sqlite3* database;
    char* databaseErrorMessage = nullptr;
    const char* tableName = "images_index";
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    int processedFiles = 0;
    sqlite3_stmt* stmt;

    if (setupDatabase(databaseFile, database, tableName, databaseErrorMessage, append, 0, stmt) !=
        0) {
        return 1;
    }

    // Iterate through files in the specified folder
    cout << "Indexing image files from folder: " << inputFolder << endl;

    for (const auto& entry : filesystem::recursive_directory_iterator(inputFolder)) {
        if (!entry.is_regular_file())
            continue;

        string extension = entry.path().extension().string();

        // Filters only image files
        if (extension != ".png" && extension != ".jpg" && extension != ".jpeg" &&
            extension != ".PNG" && extension != ".JPG" && extension != ".JPEG")
            continue;

        cout << "Processing: " << entry.path().filename().string() << endl;

        // Removes the extension, uses filename as title and content
        string filename = entry.path().stem().string();
        string title = filename;
        string cleanContent = filename;

        // Generate snippet for images (just the filename)
        string snippet = "Image: " + filename;

        // Sets relative path
        string relativePath = "/special/" + entry.path().filename().string();

        // Generates vocabulary
        cout << "  Successfully extracted vocabulary. "
             << "Vocabulary size: " << vocabulary(cleanContent, converter, vocabSet) << endl;

        // Bind values to the prepared statement
        sqlite3_bind_text(stmt, 1, relativePath.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, cleanContent.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, snippet.c_str(), -1, SQLITE_TRANSIENT);

        // Executes statement
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            cout << "  Error inserting: " << sqlite3_errmsg(database) << endl;
        }

        // Resets for next iteration
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        processedFiles++;
    }

    return finalizeDatabase(
        stmt, database, databaseErrorMessage, databaseFile, processedFiles, tableName);
}

bool vocabularyDatabase(const char* databaseFile,
                        const char* tableName,
                        set<string>& vocabSet,
                        bool append) {
    cout << "Beginning Vocabulary Transaction..." << endl;
    sqlite3* database;
    char* databaseErrorMessage = nullptr;
    sqlite3_stmt* stmt;

    if (setupDatabase(databaseFile, database, tableName, databaseErrorMessage, append, 1, stmt) !=
        0) {
        return 1;
    }

    // Inserts vocabulary into database
    string vocabString;
    for (const auto& w : vocabSet) {
        vocabString += w + " ";
    }

    sqlite3_bind_text(stmt, 1, vocabString.c_str(), -1, SQLITE_TRANSIENT);

    // Executes statement
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        cout << "  Error inserting: " << sqlite3_errmsg(database) << endl;
    }

    return finalizeDatabase(stmt, database, databaseErrorMessage, databaseFile, -1, tableName);
}

int main(int argc, const char* argv[]) {
    CommandLineParser parser(argc, argv);

    if (parser.hasOption("-help") || argc == 1)
        return helpMessage();

    // Parse command line
    bool htmlMode = 1;
    bool imageMode = 0;
    bool appendIndex = 0;
    bool appendVocab = 0;
    bool skipVocab = 0;

    // Toggles between HTML mode and Image mode
    if (parser.hasOption("-mode")) {
        if (parser.getOption("-mode") == "image") {
            htmlMode = 0;
            imageMode = 1;
        } else if (parser.getOption("-mode") == "html") {
            htmlMode = 1;
            imageMode = 0;
        }
    } else {
        cout << "error: a valid mode must be specified!" << endl;
        return helpMessage();
    }

    // Checks path validation
    if (!parser.hasOption("-path")) {
        cout << "error: a valid path must be specified!" << endl;
        return helpMessage();
    }

    // Checks whether to skip vocabulary or not
    if (parser.hasOption("-skipvocab"))
        skipVocab = 1;

    // Checks if user wants to keep old database file
    if (parser.hasOption("-append")) {
        if (parser.getOption("-append") == "index")
            appendIndex = 1;
        else if (parser.getOption("-append") == "vocab")
            appendVocab = 1;
        else if (parser.getOption("-append") == "both") {
            appendIndex = 1;
            appendVocab = 1;
        } else {
            cout << "error: invalid append value!" << endl;
            return helpMessage();
        }
    }

    // Set up variables and constants;
    string inputFolder = parser.getOption("-path");
    const char* databaseFile = htmlMode ? "index.db" : "images.db";
    char* databaseErrorMessage;
    set<string> vocabSet;

    //============================== INDEXING =============================//

    if (htmlMode) {
        if (indexDatabase(inputFolder, databaseFile, vocabSet, appendIndex))
            return 1;
    } else {
        if (imageDatabase(inputFolder, databaseFile, vocabSet, appendIndex))
            return 1;
    }

    if (!skipVocab) {
        cout << "Starting Vocabulary Indexing..." << endl;

        //============================ VOCABULARY INDEXING =============================//

        // Create FTS5 virtual table for vocabulary
        const char* vocabularyFile = htmlMode ? "index_vocab.db" : "images_vocab.db";
        char* databaseVocabErrorMessage;
        const char* tableName_vocab = imageMode ? "images_vocab" : "webpage_vocab";

        return vocabularyDatabase(vocabularyFile, tableName_vocab, vocabSet, appendVocab);
    }
    return 0;
}
