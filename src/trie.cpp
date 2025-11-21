#include "trie.h"

TrieNode* TrieNode::retrieveChild(char32_t c) {
    auto it = children.find(c);
    if (it != children.end()) {
        return it->second;
    }
    return nullptr;
}

bool TrieNode::isWordDone() {
    return isEndOfWord;
}

bool TrieNode::setEndOfWord(bool isEnd) {
    isEndOfWord = isEnd;
    return isEndOfWord;
}

bool TrieNode::insertCharacter(char32_t c) {
    if (children.find(c) == children.end()) {
        children[c] = new TrieNode();
        return true;
    }
    return false;
}

bool Trie::insert(const std::string& word) {
    TrieNode* currentNode = root;
    std::u32string utf32Word = converter.from_bytes(word);

    for (char32_t c : utf32Word) {
        currentNode->insertCharacter(c);
        currentNode = currentNode->retrieveChild(c);
    }
    return currentNode->setEndOfWord(true);
}
