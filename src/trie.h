/**
 * @file trie.h
 * @brief Browser extension for fast in-memory prefix searching
 *
 */

#ifndef TRIE_H
#define TRIE_H

#include <codecvt>
#include <locale>
#include <map>
#include <vector>
#include <string>

/**
 * @class TrieNode
 * @brief Node structure for Trie implementation
 */
class TrieNode {
  public:

    TrieNode() : isEndOfWord(false) {
    }
    ~TrieNode() {
        for (auto child : children) {
            if (child.second) {
                delete child.second;
            }
        }
    }

    /**
     * @name retrieveChild
     * @brief Retrieves child node for given character
     * @param c Character to retrieve
     * @return Pointer to child TrieNode or nullptr if not found
     */
    TrieNode* retrieveChild(char32_t c);

    /**
     * @name insertCharacter
     * @brief Inserts a character as a child node
     * @param c Character to insert
     * @return True if character was inserted, false if it already existed
     */
    bool insertCharacter(char32_t c);

    std::map<char32_t, TrieNode*> children;
    bool isEndOfWord;
};

class Trie {
  public:
    Trie() : root(new TrieNode()) {
    }
    ~Trie() {
        delete root;
    }

    /**
     * @name insert
     * @brief Inserts a word into the Trie
     * @param word The word to insert
     * @return True if insertion was successful, false otherwise
     */
    bool insert(const std::u32string& word);
    bool insert(const std::string& word);

    /**
     * @name search
     * @brief Searches for a word in the Trie
     * @param word The word to search for
     * @return True if the word exists, false otherwise
     */
    bool search(const std::u32string& word);
    bool search(const std::string& word);

    /**
     * @name startsWith
     * @brief Checks if any word in the Trie starts with the given prefix
     * @param prefix The prefix to check
     * @return True if any word starts with the prefix, false otherwise
     */
    bool startsWith(const std::u32string& prefix);
    bool startsWith(const std::string& prefix);

    /**
     * @name collectSuggestions
     * @brief Collects words in the Trie that start with the given prefix
     * @param prefix The prefix to search for
     * @param maxSuggestions Maximum number of suggestions to collect
     * @return Number of suggestions collected
     */
    size_t collectSuggestions(const std::u32string& prefix, size_t maxSuggestions);
    size_t collectSuggestions(const std::string& prefix, size_t maxSuggestions);

    // Public member to store collected words
    std::vector<std::u32string> collectWords;

  private:
    TrieNode* root;
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;

    void DFSCollector(TrieNode* node, std::u32string& currentWord, size_t& maxSuggestions);
};
#endif
