//
// Created bnode Andre Henriques on 2019-08-15.
//

#include <vector>
#include "Terminal.h"

Terminal::Terminal(long id, long node) {
    this -> id = id;
    this -> node = node;
}

Terminal::Terminal() {
    this -> id = NULL;
    this -> node = NULL;
}

vector<Terminal> Terminal::getVector(const Value &terminals) {
    vector<Terminal> terminalsVector;
    for (auto& thisTerminal : terminals.GetArray()) {
        Terminal terminal;
        if(thisTerminal.HasMember("id")) terminal.setId(thisTerminal["id"].GetUint64());
        if(thisTerminal.HasMember("node")) terminal.setNode(thisTerminal["node"].GetUint64());
        terminalsVector.push_back(terminal);
    }
    return terminalsVector;
}

long Terminal::getId() {
    return id;
}

void Terminal::setId(long id) {
    this->id = id;
}

long Terminal::getNode() {
    return node;
}

void Terminal::setNode(long node) {
    this->node = node;
}