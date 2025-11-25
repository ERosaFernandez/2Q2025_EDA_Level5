/**
 * @file edahttpd.cpp
 * @author Marc S. Ressl
 * @brief Manages the edahttpd server
 * @version 1.0
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <microhttpd.h>

#include <iostream>

#include "CommandLineParser.h"
#include "HttpRequestHandler.h"
#include "HttpServer.h"

using namespace std;

bool printHelp() {
    cout << "/==========================================================================/" << endl
         << "Parameters:" << endl
         << "-mode (image / html): mandatory," << endl
         << "defines which mode will be used." << endl
         << "-port (number): optional," << endl
         << "specifies port to run the server on. Defaults to 8000." << endl
         << "-path (insertYourFolderRelativePath): mandatory," << endl
         << "specifies relative path to the www folder." << endl
         << endl;

    cout << "example for Linux:" << endl
         << "./edahttpd -port 9000 -mode html -path ../www/" << endl
         << "example for Windows:" << endl
         << "edahttpd.exe -mode image -path ../../../../www" << endl
         << "/==========================================================================/" << endl;

    return 1;
}

int main(int argc, const char* argv[]) {
    CommandLineParser parser(argc, argv);

    if (parser.hasOption("-help"))
        return printHelp();

    // Configuration
    int port = 8000;
    string wwwPath;
    bool imageMode = false;

    // Parse command line
    if (!parser.hasOption("-path")) {
        cout << "error: the www folder path must be specified!" << endl;
        return printHelp();
    } else {
        wwwPath = parser.getOption("-path");
    }

    if (parser.hasOption("-port"))
        port = stoi(parser.getOption("-port"));

    // Detects mode
    if (parser.hasOption("-mode")) {
        string mode = parser.getOption("-mode");
        if (mode == "image") {
            imageMode = true;
            cout << "Starting in IMAGE mode" << endl;
        } else if (mode == "html")
            cout << "Starting in HTML mode" << endl;
    } else {
        cout << "a valid mode must be specified!" << endl;
        return printHelp();
    }

    // Start server
    HttpServer server(port);

    HttpRequestHandler edaOogleHttpRequestHandler(wwwPath, imageMode);
    server.setHttpRequestHandler(&edaOogleHttpRequestHandler);

    if (server.isRunning()) {
        cout << "Running server..." << endl;

        // Wait for keyboard entry
        char value;
        cin >> value;

        cout << "Stopping server..." << endl;
    }
}
