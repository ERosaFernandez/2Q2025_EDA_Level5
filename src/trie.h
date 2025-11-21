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

    TrieNode* retrieveChild(char32_t c);
    bool insertCharacter(char32_t c);
    bool isWordDone();
    bool setEndOfWord(bool isEnd);

  private:
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

    bool insert(const std::string& word);
    bool search(const std::string& word);
    bool startsWith(const std::string& prefix);
    bool collectSuggestions(const std::string& prefix, std::vector<std::string>& suggestions, size_t maxSuggestions);

    std::vector<std::u32string> collectWords;

  private:
    TrieNode* root;
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
};
#endif
