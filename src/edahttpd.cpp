/**
 * @file edahttpd.cpp
 * @author Marc S. Ressl
 * @brief Manages the edahttpd server
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <microhttpd.h>

#include <iostream>

#include "CommandLineParser.h"
#include "HttpRequestHandler.h"
#include "HttpServer.h"

using namespace std;

void printHelp() {
    cout << "Usage: edahttpd -h WWW_PATH [-p PORT] [-m MODE]" << endl;
    cout << "  -h WWW_PATH : Path to www folder" << endl;
    cout << "  -p PORT     : Server port (default: 8000)" << endl;
    cout << "  -m MODE     : Search mode ('image' or 'images' for image search, default: HTML mode)" << endl;
}

int main(int argc, const char* argv[]) {
    CommandLineParser parser(argc, argv);

    // Configuration
    int port = 8000;
    string wwwPath;
    bool imageMode = false;

    // Parse command line
    if (!parser.hasOption("-h")) {
        cout << "error: WWW_PATH must be specified." << endl;

        printHelp();

        return 1;
    }

    wwwPath = parser.getOption("-h");

    if (parser.hasOption("-p"))
        port = stoi(parser.getOption("-p"));

    // Detects mode
    if (parser.hasOption("-m")) {
        string mode = parser.getOption("-m");
        if (mode == "image" || mode == "images") {
            imageMode = true;
            cout << "Starting in IMAGE mode" << endl;
        }
        else cout << "Starting in HTML mode" << endl;
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
