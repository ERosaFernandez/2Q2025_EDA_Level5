/**
 * @file mkindex.cpp
 * @author Marc S. Ressl
 * @brief Makes a database index
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <sqlite3.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "CommandLineParser.h"

using namespace std;

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

int main(int argc, const char* argv[]) {
    CommandLineParser parser(argc, argv);

    // Parse command line
    bool htmlMode = parser.hasOption("-w");
    bool imageMode = parser.hasOption("-s");

	//Toggles between HTML mode and Image mode
    if (!htmlMode && !imageMode) {
        cout << "error: input folder must be specified!" << endl
             << "HTML mode: ./mkindex -w ../www/" << endl
             << "Image mode: ./mkindex -s ../special/" << endl;
        return 1;
    }

    if (htmlMode && imageMode) {
        cout << "error: cannot use -w and -s at the same time!" << endl;
        return 1;
    }

    string inputFolder = htmlMode ? parser.getOption("-w") : parser.getOption("-s");
    const char* databaseFile = htmlMode? "index.db": "images.db";
    sqlite3* database;
    char* databaseErrorMessage;

    // Open database file
    cout << "Opening database..." << endl;
    if (sqlite3_open(databaseFile, &database) != SQLITE_OK) {
        cout << "Can't open database: " << sqlite3_errmsg(database) << endl;

        return 1;
    }



    // Create FTS5 virtual table
    const char* tableName = imageMode ? "images_index" : "webpage_index";
    cout << "Creating FTS5 virtual table: " << tableName << "..." << endl;

    string createTableSQL = string("CREATE VIRTUAL TABLE IF NOT EXISTS ") + tableName +
                            " USING fts5("
                            "path UNINDEXED,"
                            "title,"
                            "content);";

    if (sqlite3_exec(database,
                     createTableSQL.c_str(),
                     NULL,
                     0,
                     &databaseErrorMessage) != SQLITE_OK) {
        cout << "Error: " << sqlite3_errmsg(database) << endl;
        sqlite3_close(database);
        return 1;
    }

    // Delete previous entries if table already existed
    cout << "Deleting previous entries..." << endl;
    string deleteSQL = string("DELETE FROM ") + tableName + ";";
    if (sqlite3_exec(database, deleteSQL.c_str(), NULL, 0, &databaseErrorMessage) !=
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
    sqlite3_stmt* stmt;
    string insertSQL =
        string("INSERT INTO ") + tableName + " (path, title, content) VALUES (?, ?, ?);";
    //const char* sql = "INSERT INTO webpage_index (path, title, content) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(database, insertSQL.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        cout << "Error preparing statement: " << sqlite3_errmsg(database) << endl;
        sqlite3_close(database);
        return 1;
    }

    // Iterate through files in the specified folder
    const char* fileType = imageMode ? "image" : "HTML";
    cout << "Indexing " << fileType << " files from folder: " << inputFolder << endl;

    int processedFiles = 0;

	if (htmlMode){
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

                // Exrtact title
                string title = "No Title";
                size_t titleStart = htmlContent.find("<title>");
                size_t titleEnd = htmlContent.find("</title>");

                if (titleStart != string::npos && titleEnd != string::npos &&
                    titleEnd > titleStart) {
                    title = htmlContent.substr(titleStart + 7, titleEnd - (titleStart + 7));
                }

                // Parse HTML content to plain text
                string cleanContent = removeHTMLTags(htmlContent);  // ← Función que implementarás

                // Sets relative path
                string relativePath = "/wiki/" + entry.path().filename().string();

                // Bind values to the prepared statement
                sqlite3_bind_text(stmt, 1, relativePath.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, cleanContent.c_str(), -1, SQLITE_TRANSIENT);

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
    }
	else {
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

            // Sets relative path
            string relativePath = "/special/" + entry.path().filename().string();

            // Bind values to the prepared statement
            sqlite3_bind_text(stmt, 1, relativePath.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, cleanContent.c_str(), -1, SQLITE_TRANSIENT);

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


    // Clears statement
    sqlite3_finalize(stmt);

    // Commits transaction
    cout << "Committing transaction..." << endl;
    sqlite3_exec(database, "COMMIT;", NULL, 0, NULL);

    cout << "Successfully indexed " << processedFiles << " files." << endl;

    // Close database
    cout << "Closing database..." << endl;
    sqlite3_close(database);
}
