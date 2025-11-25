/**
 * @file trie.h
 * @brief Browser extension for fast in-memory prefix searching
 * @version 1.0
 */

#include "trie.h"

#include <unicode/uchar.h>
#include <unicode/ustring.h>

TrieNode* TrieNode::retrieveChild(char32_t c) {
    // Searches for child node
    auto it = children.find(c);
    if (it != children.end()) {
        return it->second;
    }
    return nullptr;
}

bool TrieNode::insertCharacter(char32_t c) {
    // Inserts character if not already present
    if (children.find(c) == children.end()) {
        children[c] = new TrieNode();
        return true;
    }
    return false;
}

bool Trie::insert(const std::u32string& word) {
    TrieNode* currentNode = root;

    // Inserts each character of the word
    for (char32_t c : word) {
        currentNode->insertCharacter(c);
        currentNode = currentNode->retrieveChild(c);
    }
    return currentNode->isEndOfWord = true;
}

bool Trie::insert(const std::string& word) {
    // Converts to UTF-32 and inserts
    std::u32string utf32Word = converter.from_bytes(word);
    return insert(utf32Word);
}

bool Trie::search(const std::u32string& word) {
    TrieNode* currentNode = root;

    // Traverses the Trie for each character
    for (char32_t c : word) {
        currentNode = currentNode->retrieveChild(c);
        if (!currentNode) {
            return false;
        }
    }
    return currentNode->isEndOfWord;
}

bool Trie::search(const std::string& word) {
    // Converts to UTF-32 and searches
    std::u32string utf32Word = converter.from_bytes(word);
    return search(utf32Word);
}

bool Trie::startsWith(const std::u32string& prefix) {
    TrieNode* currentNode = root;

    // Traverses the Trie for each character
    for (char32_t c : prefix) {
        currentNode = currentNode->retrieveChild(c);
        if (!currentNode) {
            return false;
        }
    }
    return true;
}

bool Trie::startsWith(const std::string& prefix) {
    // Converts to UTF-32 and checks prefix
    std::u32string utf32Prefix = converter.from_bytes(prefix);
    return startsWith(utf32Prefix);
}

size_t Trie::collectSuggestions(const std::u32string& prefix, size_t maxSuggestions) {
    collectWords.clear();

    // Checks if prefix exists
    if (!startsWith(prefix) || maxSuggestions == 0) {
        return 0;
    }

    TrieNode* currentNode = root;
    std::u32string currentPrefix;

    // Traverses to the end of the prefix
    for (char32_t c : prefix) {
        currentPrefix.push_back(u_tolower(c));
        currentNode = currentNode->retrieveChild(c);
    }

    // Performs DFS to collect suggestions
    DFSCollector(currentNode, currentPrefix, maxSuggestions);

    return collectWords.size();
}

size_t Trie::collectSuggestions(const std::string& prefix, size_t maxSuggestions) {
    // Converts to UTF-32 and collects suggestions
    std::u32string utf32Prefix = converter.from_bytes(prefix);
    return collectSuggestions(utf32Prefix, maxSuggestions);
}

void Trie::DFSCollector(TrieNode* node, std::u32string& currentWord, size_t& maxSuggestions) {
    // Returns if reached max suggestions
    if (!maxSuggestions) {
        return;
    }

    // If end of word, add to suggestions
    if (node->isEndOfWord) {
        collectWords.push_back(currentWord);
        maxSuggestions--;
    }

    // Traverse children
    for (auto& childIterator : node->children) {
        // Add character to current word and recurse
        currentWord.push_back(u_tolower(childIterator.first));
        DFSCollector(childIterator.second, currentWord, maxSuggestions);
        // Goes back one character
        currentWord.pop_back();
        // Avoids unnecessary traversals
        if (!maxSuggestions) {
            return;
        }
    }
}
