//
// Created by Andre Henriques on 2019-08-13.
//

#ifndef SAM_COORDINATOR_ARM_EXTERNALCONFIGURATION_HPP
#define SAM_COORDINATOR_ARM_EXTERNALCONFIGURATION_HPP

#include <iostream>
#include <configuration/Utility/Utility.hpp>
#include <configuration/Communication/Communication.h>
#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

class ExternalConfiguration {
private:
    string samid;
    string version;
    vector<Communication> communications;
    Utility utility; // = Utility(0, "Utility Name", {});
    vector<ExternalConfiguration> externalConfigurations;

public:
    ExternalConfiguration();

    vector<ExternalConfiguration> getVector(const Value &externalConfigurations);

    const string &getSamId() const;

    void setSamId(const string &samid);

    const string &getVersion() const;

    void setVersion(const string &version);

    const Utility &getUtility() const;

    void setUtility(const Value &utility);

    const vector<ExternalConfiguration> &getExternalConfigurations() const;

    void setExternalConfigurations(const Value &externalConfigurations);

    const vector<Communication> &getCommunications() const;

    void setCommunications(const Value &communications);
};


#endif //SAM_COORDINATOR_ARM_EXTERNALCONFIGURATION_HPP
