/**
 * @file HttpResponses.h
 * @brief EDAoggle search engine webpage responses
 * @version 1.0
 *
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

    inline string imagePageResponse(string cleanedTitle,
                                    string encodedImageUrl,
                                    string filename,
                                    string cleanUrlStr) {
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
                :root {\
                    --bg: #ffffff;\
                    --text: #202124;\
                    --shadow: rgba(0,0,0,0.1);\
                    --accent: #4285f4;\
                }\
                @media (prefers-color-scheme: dark) {\
                    :root {\
                        --bg: #202124;\
                        --text: #e8eaed;\
                        --shadow: rgba(0,0,0,0.3);\
                    }\
                }\
                body {\
                    font-family: Inter, sans-serif;\
                    margin: 0;\
                    padding: 0;\
                    background: var(--bg);\
                    color: var(--text);\
                    display: flex;\
                    flex-direction: column;\
                    min-height: 100vh;\
                }\
                .image-viewer {\
                    max-width: 1200px;\
                    margin: 2rem auto;\
                    padding: 0 2rem;\
                    flex-grow: 1;\
                }\
                .image-header {\
                    display: flex;\
                    justify-content: space-between;\
                    align-items: center;\
                    margin-bottom: 1rem;\
                }\
                .back-link {\
                    color: var(--accent);\
                    text-decoration: none;\
                    font-weight: 600;\
                }\
                .image-title {\
                    font-size: 1.5rem;\
                    font-weight: 600;\
                }\
                .image-main {\
                    position: relative;\
                    margin-bottom: 2rem;\
                    border-radius: 12px;\
                    overflow: hidden;\
                    box-shadow: 0 4px 16px var(--shadow);\
                    transition: all 0.3s ease;\
                }\
                .image-main img {\
                    width: 100%;\
                    height: auto;\
                    display: block;\
                    object-fit: contain;\
                    max-height: 80vh;\
                    loading: lazy;\
                }\
                .zoom-controls {\
                    position: absolute;\
                    bottom: 1rem;\
                    right: 1rem;\
                    display: flex;\
                    gap: 0.5rem;\
                }\
                .zoom-btn {\
                    background: rgba(255,255,255,0.8);\
                    border: none;\
                    padding: 0.5rem;\
                    border-radius: 50%;\
                    cursor: pointer;\
                    transition: background 0.2s;\
                }\
                .zoom-btn:hover {\
                    background: rgba(255,255,255,1);\
                }\
                .image-details {\
                    background: rgba(0,0,0,0.05);\
                    padding: 1rem;\
                    border-radius: 8px;\
                }\
                .detail-item {\
                    margin-bottom: 0.5rem;\
                }\
                .detail-label {\
                    font-weight: 600;\
                }\
                @media (max-width: 768px) {\
                    .image-viewer {\
                        padding: 0 1rem;\
                    }\
                    .image-main img {\
                        max-height: 60vh;\
                    }\
                }\
            </style>\
        </head>\
        <body>\
            <div class=\"image-viewer\">\
                <div class=\"image-header\">\
                    <a href=\"/\" class=\"back-link\">Volver a EDAoogle</a>\
                    <h1 class=\"image-title\">") +
               cleanedTitle +
               string(
                   "</h1>\
                </div>\
                <div class=\"image-main\" role=\"region\" aria-label=\"Vista de imagen\">\
                    <img src=\"") +
               encodedImageUrl + string("\" alt=\"") + cleanedTitle +
               string(
                   " - Imagen detallada\" loading=\"lazy\">\
                    <div class=\"zoom-controls\">\
                        <button class=\"zoom-btn\" onclick=\"zoomIn()\">+</button>\
                        <button class=\"zoom-btn\" onclick=\"zoomOut()\">-</button>\
                        <button class=\"zoom-btn\" onclick=\"fullScreen()\">⤢</button>\
                    </div>\
                </div>\
                <div class=\"image-details\">\
                    <div class=\"detail-item\"><span class=\"detail-label\">Nombre del archivo:</span> ") +
               filename +
               string(
                   "</div>\
                    <div class=\"detail-item\"><span class=\"detail-label\">Ubicacion:</span> ") +
               cleanUrlStr +
               string(
                   "</div>\
                </div>\
            </div>\
            <script>\
                let zoomLevel = 1;\
                const img = document.querySelector('.image-main img');\
                function zoomIn() {\
                    zoomLevel += 0.1;\
                    img.style.transform = `scale(${zoomLevel})`;\
                }\
                function zoomOut() {\
                    zoomLevel = Math.max(1, zoomLevel - 0.1);\
                    img.style.transform = `scale(${zoomLevel})`;\
                }\
                function fullScreen() {\
                    if (img.requestFullscreen) img.requestFullscreen();\
                }\
            </script>\
        </body>\
        </html>");
    }

    inline string searchPageStart(string searchString) {
        return R"(
        <!DOCTYPE html>
        <html lang="es">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>EDAoogle - Resultados</title>
            <link rel="preload" href="https://fonts.googleapis.com" />
            <link rel="preload" href="https://fonts.gstatic.com" crossorigin />
            <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600&display=swap" rel="stylesheet" />
            <link rel="preload" href="css/style.css" />
            <link rel="stylesheet" href="css/style.css" />
            <style>
                :root {
                    --google-blue: #4285f4;
                    --shadow: rgba(0, 0, 0, 0.1);
                    --text: #202124;
                    --input-bg: #ffffff;
                    --bg-light: #fcfcfc;
                }

                @media (prefers-color-scheme: dark) {
                    :root {
                        --google-blue: #4285f4;
                        --shadow: rgba(0, 0, 0, 0.3);
                        --text: #e8eaed;
                        --input-bg: #303134;
                        --bg-light: #202124;
                    }
                }

                body {
                    font-family: Inter, sans-serif;
                    margin: 0;
                    padding: 0;
                    background: var(--bg-light);
                    color: var(--text);
                }

                header {
                    display: flex;
                    align-items: center;
                    padding: 16px 24px;
                    border-bottom: 1px solid rgba(128, 128, 128, 0.1);
                    gap: 24px;
                    background: var(--bg-light); /* Integrate with background */
                }

                .logo-small {
                    font-size: 1.5rem;
                    font-weight: 600;
                    text-decoration: none;
                }

                .logo-small span:nth-child(1) { color: #4285f4; }
                .logo-small span:nth-child(2) { color: #ea4335; }
                .logo-small span:nth-child(3) { color: #fbbc04; }
                .logo-small span:nth-child(4) { color: #4285f4; }
                .logo-small span:nth-child(5) { color: #34a853; }
                .logo-small span:nth-child(6) { color: #ea4335; }
                .logo-small span:nth-child(7) { color: #a142f4; }
                .logo-small span:nth-child(8) { color: #24c1e0; }

                .search-header {
                    flex: 1;
                    max-width: 600px;
                }

                form {
                    position: relative;
                }

                .search-container {
                    background: var(--input-bg);
                    border-radius: 24px;
                    box-shadow: 0 1px 6px var(--shadow);
                    overflow: hidden;
                    border: 1px solid #dfe1e5; /* Added subtle border for integration */
                    transition: box-shadow 0.2s ease; /* Smooth hover effect */
                }

                .search-container:hover {
                    box-shadow: 0 2px 8px var(--shadow);
                }

                .search-input-wrapper {
                    display: flex;
                    align-items: center;
                    padding: 0 16px;
                }

                input[type="text"] {
                    flex: 1;
                    border: none;
                    outline: none;
                    padding: 12px 0;
                    font-size: 16px;
                    background: transparent;
                    color: var(--text);
                }

                button[type="submit"] {
                    padding: 8px 16px;
                    background: var(--google-blue);
                    color: white;
                    border: none;
                    border-radius: 4px;
                    cursor: pointer;
                    font-size: 14px;
                }

                #suggestions {
                    position: absolute;
                    background: var(--input-bg);
                    border-radius: 8px;
                    box-shadow: 0 2px 8px var(--shadow);
                    width: 100%;
                    max-height: 300px;
                    overflow-y: auto;
                    display: none;
                    z-index: 10000;
                    top: calc(100% + 4px);
                }

                #suggestions.show {
                    display: block;
                }

                #suggestions-overlay {
                    position: fixed;
                    top: 0;
                    left: 0;
                    width: 100%;
                    height: 100%;
                    background: transparent;
                    z-index: 9999;
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
                    background: var(--bg-light); /* Consistent with body */
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
                    flex-direction: row-reverse; /* Image on the right */
                    justify-content: flex-start;
                }

                .image-thumbnail {
                    flex-shrink: 0;
                    border-radius: 12px;
                    overflow: hidden;
                    box-shadow: 0 2px 8px var(--shadow);
                    transition: all 0.3s ease;
                    align-self: stretch; /* Stretch to match text height */
                }

                .image-thumbnail:hover {
                    transform: scale(1.05);
                    box-shadow: 0 4px 16px var(--shadow);
                }

                .image-thumbnail img {
                    max-width: 300px; /* Larger width */
                    height: 100%; /* Fill container height */
                    object-fit: contain; /* Maintain aspect ratio without cropping */
                    display: block;
                }

                .image-details {
                    flex: 1;
                    min-height: 150px; /* Approximate height for text block */
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
                        flex-direction: column-reverse; /* Image below on mobile for better flow */
                    }

                    .image-thumbnail img {
                        width: 100%;
                        max-width: none;
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

            <main>
    )";
    }

    inline string searchPageEnd() {
        return R"(
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
}  // namespace Responses

#endif
