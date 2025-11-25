# EDAoggle -- Local Search Engine

EDAoggle es un motor de búsqueda local diseñado para indexar y consultar
contenido HTML e imágenes utilizando **SQLite FTS5**, un sistema de
**autocompletado basado en Trie**, y **generación automática de
snippets**. Incluye un manejador HTTP integrado para servir resultados
desde el navegador.

## Funcionalidades principales

### Indexación (mkindex)

-   **Modos disponibles:**
    -   **HTML (`-mode html`)**: limpieza del documento HTML, extracción
        de título, generación de snippet y vocabulario.
    -   **Imágenes (`-mode image`)**: indexación de archivos `.png`,
        `.jpg`, `.jpeg` con snippet descriptivo.
-   **Bases generadas:**
    -   HTML → `index.db`, `index_vocab.db`
    -   Imágenes → `images.db`, `images_vocab.db`
-   **Opciones adicionales:**
    -   `-append index/vocab/both` -- conserva y amplía bases existentes
    -   `-skipvocab` -- omite la generación del vocabulario
    -   `-path` -- ruta del contenido a indexar

## Implementaciones destacadas

-   **Snippets automáticos:**\
    HTML → limpieza completa (remueve scripts, styles, tags) y
    extracción de texto útil.\
    Imágenes → snippet generado a partir del nombre del archivo.

-   **Autocompletado basado en Trie:**\
    Construido desde el vocabulario generado; disponible mediante
    `/predict?q=`.

-   **Búsqueda mediante SQLite FTS5:**\
    Resultados rankeados por relevancia utilizando BM25.

-   **"I'm Feeling Lucky":**\
    Selección aleatoria eficiente usando `rowid` + `RANDOM()`.

-   **Visor de imágenes integrado:**\
    Accesible con `?view=1`, mostrando título, URL limpia y la imagen.

## Observación / Aclaración
    Para ejecutar ambos programas es necesario instalar ICU, vital
    para procesado de simbolos UNICODE. Instalación con vcpkg:

    vcpkg install ICU

## Ejemplos de uso

### Linux

HTML:

    ./mkindex -mode html -path ../www/
    ./edahttpd -mode html -path ../www/

Imágenes:

    ./mkindex -mode image -path ../www/special/
    ./edahttpd -mode image -path ../www/

### Windows (CMD o PowerShell)

HTML:

    mkindex.exe -mode html -path ..\..\..\..\www\
    edahttpd.exe -mode html -path ..\..\..\..\www\

Imágenes:

    mkindex.exe -mode image -path ..\..\..\..\www\special\
    edahttpd.exe -mode image -path ..\..\..\..\www\

*una vez corriendo, el servidor se cierra apretando cualquier tecla + enter*
