//
// Created by Andre Henriques on 2019-08-13.
//

#include "ExternalConfiguration.hpp"

using namespace std;
using namespace rapidjson;

ExternalConfiguration::ExternalConfiguration() {
    vector<Communication> _communications;
    samid = "NA";
    version = "0.0.0";
    communications = _communications;
    utility = Utility();
}

vector<ExternalConfiguration> ExternalConfiguration::getVector(const Value &externalConfigurations) {
    vector<ExternalConfiguration> externalConfigurationsVector;
    for(auto &thisExternalConfigurations : externalConfigurations.GetArray()) {
        ExternalConfiguration externalConfiguration;
        if(thisExternalConfigurations.HasMember("samId")) externalConfiguration.setSamId(thisExternalConfigurations["samId"].GetString());
        if(thisExternalConfigurations.HasMember("version")) externalConfiguration.setVersion(thisExternalConfigurations["version"].GetString());
        if(thisExternalConfigurations.HasMember("communications")) externalConfiguration.setCommunications(thisExternalConfigurations["communications"]);
        if(thisExternalConfigurations.HasMember("utility")) externalConfiguration.setUtility(thisExternalConfigurations["utility"]);
        if(thisExternalConfigurations.HasMember("externalConfigurations")) externalConfiguration.setExternalConfigurations(thisExternalConfigurations["externalConfigurations"]);
        externalConfigurationsVector.push_back(externalConfiguration);
    }
    return externalConfigurationsVector;
}

const string &ExternalConfiguration::getSamId() const {
    return samid;
}

void ExternalConfiguration::setSamId(const string &samid) {
    ExternalConfiguration::samid = samid;
}

const string &ExternalConfiguration::getVersion() const {
    return version;
}

void ExternalConfiguration::setVersion(const string &version) {
    ExternalConfiguration::version = version;
}

const Utility &ExternalConfiguration::getUtility() const {
    return utility;
}

void ExternalConfiguration::setUtility(const Value &utility) {
    Utility utilityObject = Utility(utility);
    this->utility = utilityObject;
}

const vector<Communication> &ExternalConfiguration::getCommunications() const {
    return communications;
}

void ExternalConfiguration::setCommunications(const Value &_communications) {
    Communication communication;
    vector<Communication> communicationsVector;
    communicationsVector = communication.getVector(_communications);
    communications = communicationsVector;
}

const vector<ExternalConfiguration> &ExternalConfiguration::getExternalConfigurations() const {
    return externalConfigurations;
}

void ExternalConfiguration::setExternalConfigurations(const Value &_externalConfiguration) {
    ExternalConfiguration externalConfiguration;
    vector<ExternalConfiguration> externalConfigurationVector;
    externalConfigurationVector = externalConfiguration.getVector(_externalConfiguration);
    externalConfigurations = externalConfigurationVector;
}
