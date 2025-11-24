/**
 * @file HttpRequestHandler.h
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.5
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#ifndef HTTPRESPONSES_H
#define HTTPRESPONSES_H

#include <unicode/uchar.h>
#include <unicode/ustring.h>

#include <string>

namespace Responses {
	using namespace std;


	inline string homePageResponse() {
		return string(
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
	}

    inline string imagePageResponse(string cleanedTitle, string encodedImageUrl, string filename, string cleanUrlStr) {
        return string(
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
    }

    inline string searchPageStart(string searchString)
    {
        return R"(<!DOCTYPE html>
            <head>
            <meta charset = "UTF-8">
            <meta name = "viewport" content = "width=device-width, initial-scale=1.0">
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
            
    }

    inline string searchPageEnd()
    {
        return R"(
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
    }
}

#endif