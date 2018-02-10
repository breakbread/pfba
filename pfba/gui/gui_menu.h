//
// Created by cpasjuste on 30/01/18.
//

#ifndef PFBA_GUI_MENU_H
#define PFBA_GUI_MENU_H

#include "skeleton/rectangle.h"
#include "gui.h"

class MenuLine;

class GuiMenu : public c2d::Rectangle {

public:

    GuiMenu(Gui *gui);

    ~GuiMenu();

    int update();

    void loadMenu(bool isRomMenu = false, OptionMenu *om = NULL);

private:

    bool isOptionHidden(Option *option);

    Gui *gui = NULL;
    c2d::Text *title = NULL;
    c2d::Rectangle *highlight = NULL;
    std::vector<MenuLine *> lines;
    c2d::Color fillColor[2] = {
            {55, 55, 55, 230},
            {55, 55, 55, 180}};

    OptionMenu *optionMenuGui = NULL;
    OptionMenu *optionMenuRom = NULL;
    OptionMenu *optionMenu = NULL;
    std::vector<Option> *options;
    int optionIndex = 0;
    int optionCount = 0;
    bool isRomMenu = false;
    bool isEmuRunning = false;
};

#endif //PFBA_GUI_MENU_H
