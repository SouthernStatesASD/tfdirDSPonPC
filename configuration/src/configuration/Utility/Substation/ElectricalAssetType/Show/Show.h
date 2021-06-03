//
// Created by Andre Henriques on 2019-08-15.
//

#ifndef SAM_COORDINATOR_ARM_SHOW_H
#define SAM_COORDINATOR_ARM_SHOW_H

#include <iostream>

using namespace std;

class Show {
private:
    bool cellColor;
    bool icon;
    bool value;
public:
    Show();

    bool isCellColor() const;

    void setCellColor(bool cellColor);

    bool isIcon() const;

    void setIcon(bool icon);

    bool isValue() const;

    void setValue(bool value);
};


#endif //SAM_COORDINATOR_ARM_SHOW_H
