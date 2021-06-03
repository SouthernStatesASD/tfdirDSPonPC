//
// Created by Andre Henriques on 2019-08-08.
//

#ifndef SAM_COORDINATOR_ARM_CONFIGURATION_HPP
#define SAM_COORDINATOR_ARM_CONFIGURATION_HPP

#include <iostream>
#include <configuration/Utility/Utility.hpp>
#include <configuration/ExternalConfiguration/ExternalConfiguration.hpp>
#include "rapidjson/document.h"
#include "configuration/Communication/Communication.h"

using namespace std;
using namespace rapidjson;

class Configuration {
private:
    string samid = "NA";
    string version = "0.0.0";
    vector<Communication> communications = {};
    Utility utility = Utility(0, "Utility Name", {});
    vector<ExternalConfiguration> externalConfigurations = {};
public:
    Configuration(const string& configurationFileName = "configuration.json", const bool defaultPath = true);

    const vector<ExternalConfiguration> getNeighbors(const Configuration &configuration) const;

    const vector<ExternalConfiguration> getN1Neighbors(const Configuration &configuration) const;

    const string &getSamId() const;

    void setSamId(const string &samid);

    const string &getVersion() const;

    void setVersion(const string &version);

    const vector<Communication> &getCommunications() const;

    void setCommunications(const Value &communications);

    const Utility getUtility() const;

    void setUtility(const Utility &utility);

    const vector<ExternalConfiguration> &getExternalConfigurations() const;

    void setExternalConfigurations(const vector<ExternalConfiguration> &externalConfigurations);
};


#endif //SAM_COORDINATOR_ARM_CONFIGURATION_HPP
