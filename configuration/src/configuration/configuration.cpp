//
// Created by Andre Henriques on 2019-08-08.
//

#include "configuration.hpp"
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/filereadstream.h"
#include <cstdio>
#include <utility>
#include <vector>
#include <configuration/Utility/Substation/Substation.hpp>
#include "configuration/Utility/Utility.hpp"
#include "configuration/ExternalConfiguration/ExternalConfiguration.hpp"

using namespace std;
using namespace rapidjson;

Configuration::Configuration(const string &configurationFileName, const bool defaultPath) {
    /**
     * Read from a file
     */

    string filename = "";
     if (defaultPath) {
#ifdef _WIN64
         // do Windows-specific stuff
         filename = "C:\\\\sam\\";
#else
         // do Unix-specific stuff
         filename = "/opt/sam/";
#endif
     }
    filename.append(configurationFileName);
    FILE *fp = fopen(filename.c_str(), "r"); // non-Windows use "r"
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document document;
    document.ParseStream(is);
    fclose(fp);

    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    document.Accept(writer);    // Accept() traverses the DOM and generates Handler events.

    if (document.HasMember("samId")) this->samid = document["samId"].GetString();
    if (document.HasMember("version")) this->version = document["version"].GetString();
    if (document.HasMember("communications")) {
        Value communications;
        Communication communication;
        vector<Communication> communicationsVector;
        communications = document["communications"];
        communicationsVector = communication.getVector(communications);
        this->communications = communicationsVector;
    }
    if (document.HasMember("utility")) {
        Value utility;
        Utility utilityObject;
        utility = document["utility"];
        utilityObject = Utility(utility);
        this->utility = utilityObject;
    }
    if (document.HasMember("externalConfigurations")) {
        Value externalConfigurations;
        ExternalConfiguration externalConfiguration;
        vector<ExternalConfiguration> externalConfigurationsVector;
        externalConfigurations = document["externalConfigurations"];
        externalConfigurationsVector = externalConfiguration.getVector(externalConfigurations);
        this->externalConfigurations = externalConfigurationsVector;
    }
}

const vector<ExternalConfiguration> Configuration::getNeighbors(const Configuration &configuration) const {
    vector<ExternalConfiguration> neighbors;
    vector<Terminal> neighborTerminals;

    vector<Substation> substations = configuration.getUtility().getSubstations();

    // get the SAM's neighboring terminals.
    for (Substation substation : substations) {
        vector<ElectricalAsset> electricalAssets = substation.getElectricalAssets();
        for (const ElectricalAsset &electricalAsset : electricalAssets) {
            vector<Terminal> terminals = electricalAsset.getTerminals();
            for (Terminal terminal : terminals) {
                neighborTerminals.push_back(terminal);
            }
        }
    }

    // Find assets with shared terminal nodes
    vector<ExternalConfiguration> _externalConfigurations = configuration.getExternalConfigurations();
    for (auto &thisConfiguration : _externalConfigurations) {
        bool addNeighbor = false;
        vector<Substation> externalSubstations = thisConfiguration.getUtility().getSubstations();

        for (auto &substation : externalSubstations) {
            vector<ElectricalAsset> electricalAssets = substation.getElectricalAssets();
            for (auto &asset : electricalAssets) {

                vector<Terminal> terminals = asset.getTerminals();
                for (auto &terminal : terminals) {
                    for (auto &neighborTerminal : neighborTerminals) {
                        if (terminal.getNode() == neighborTerminal.getNode()) {
                            addNeighbor = true;
                        }
                    }
                }
            }
        }

        if (addNeighbor) {
            neighbors.push_back(thisConfiguration);
        }
    }

    return neighbors;
}

const vector<ExternalConfiguration> Configuration::getN1Neighbors(const Configuration &configuration) const {
    vector<ExternalConfiguration> n1Neighbors;

    vector<ExternalConfiguration> _externalConfigurations = configuration.getExternalConfigurations();
    vector<ExternalConfiguration> _externalN1Configurations;

    for (auto &thisConfiguration : _externalConfigurations) {
        _externalN1Configurations = thisConfiguration.getExternalConfigurations();

        for (auto &n1Configuration : _externalN1Configurations) {
            n1Neighbors.push_back(n1Configuration);
        }
    }

    return n1Neighbors;
}

const string &Configuration::getSamId() const {
    return samid;
}

void Configuration::setSamId(const string &samid) {
    Configuration::samid = samid;
}

const string &Configuration::getVersion() const {
    return version;
}

void Configuration::setVersion(const string &version) {
    Configuration::version = version;
}

const Utility Configuration::getUtility() const {
    return utility;
}

void Configuration::setUtility(const Utility &utility) {
    Configuration::utility = utility;
}

const vector<ExternalConfiguration> &Configuration::getExternalConfigurations() const {
    return externalConfigurations;
}

void Configuration::setExternalConfigurations(const vector<ExternalConfiguration> &externalConfigurations) {
    Configuration::externalConfigurations = externalConfigurations;
}

const vector<Communication> &Configuration::getCommunications() const {
    return communications;
}

void Configuration::setCommunications(const Value &_communications) {
    Communication communication;
    vector<Communication> communicationsVector;
    communicationsVector = communication.getVector(_communications);
    communications = communicationsVector;
}
