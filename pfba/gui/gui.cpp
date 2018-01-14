//
// Created by cpasjuste on 22/11/16.
//
#include <skeleton/timer.h>
#include <algorithm>
#include "run.h"
#include "video.h"
#include "menu.h"

using namespace c2d;

extern Video *video;

extern INT32 MakeScreenShot(const char *dst);

static int option_index = 0;

class GuiRomInfo : public Rectangle {

public:

    GuiRomInfo(const FloatRect &rect, float scale) : Rectangle(rect) {

        setFillColor(Color::Transparent);
        scaling = scale;

        rectangle = new Rectangle(
                FloatRect(0, getSize().y / 2 + UI_MARGIN * scaling,
                          getSize().x, getSize().y / 2 - UI_MARGIN * scaling));
        rectangle->setFillColor(Color::GrayLight);
        rectangle->setOutlineColor(COL_GREEN);
        rectangle->setOutlineThickness(2);
        add(rectangle);
    }

    void update(RomList::Rom *rom) {

        if (texture) {
            delete (texture);
            texture = NULL;
        }

        if (!rom) {

        } else {
            // load preview image
            char path[MAX_PATH];
            sprintf(path, "%s/%s.png", szAppPreviewPath, rom->zip);
            texture = new C2DTexture(path);
            if (!texture->available && rom->parent) {
                // try parent image
                delete (texture);
                memset(path, 0, MAX_PATH);
                sprintf(path, "%s/%s.png", szAppPreviewPath, rom->parent);
                texture = new C2DTexture(path);
            }

            // set preview image
            if (texture->available) {
                float outline = rectangle->getOutlineThickness() * 2;
                texture->setOutlineColor(COL_YELLOW);
                texture->setOutlineThickness(outline);
                texture->setOriginCenter();
                texture->setPosition(Vector2f(getLocalBounds().width / 2, getLocalBounds().height / 4));
                float tex_scaling = std::min(
                        (getLocalBounds().width - UI_MARGIN * scaling + outline * 2) / texture->getSize().x,
                        ((getLocalBounds().height / 2) - UI_MARGIN * scaling + outline * 2) / texture->getSize().y);
                texture->setScale(tex_scaling, tex_scaling);
                add(texture);
            }


        }
    }

    Texture *texture = NULL;
    Rectangle *rectangle;
    float scaling = 1;
};

Gui::Gui(Io *i, Renderer *r, Skin *s, RomList *rl, Config *cfg, Input *in)
        : Rectangle(r->getSize()) {

    io = i;
    renderer = r;
    skin = s;
    romList = rl;
    config = cfg;
    input = in;

    // filter roms
    FilterRoms();

    // build menus from options
    menu_gui = new Menu(NULL, cfg->GetGuiOptions());
    menu_rom = new Menu(NULL, cfg->GetRomOptions(), true);
    menu_current = menu_gui;

    // scaling factor mainly used for borders
    // based on vita resolution..
    scaling = std::min(renderer->getSize().x / 960, 1.0f);

    // set gui main "window"
    setFillColor(Color::Gray);
    setOutlineColor(COL_ORANGE);
    setOutlineThickness(scaling < 1 ? 1 : 2);
    setPosition(getOutlineThickness(), getOutlineThickness());
    setSize(Vector2f(r->getSize().x - getOutlineThickness() * 2, r->getSize().y - getOutlineThickness() * 2));
    renderer->add(this);

    // add title image if available
    if (skin->tex_title->available) {
        skin->tex_title->setPosition(UI_MARGIN * scaling, UI_MARGIN * scaling);
        float scale = (getLocalBounds().width / 3) / skin->tex_title->getSize().x;
        skin->tex_title->setScale(scale, scale);
        renderer->add(skin->tex_title);
    }

    // add rom list ui
    float top = skin->tex_title->getGlobalBounds().top
                + skin->tex_title->getGlobalBounds().height
                + UI_MARGIN * scaling;

    FloatRect rect = {
            UI_MARGIN * scaling, top,
            (getLocalBounds().width / 2) - UI_MARGIN * scaling,
            getLocalBounds().height - top - UI_MARGIN * scaling};

    guiRomList = new ListBox(*skin->font, rect, (std::vector<Io::File *> &) roms);
    guiRomList->setOutlineThickness(getOutlineThickness());
    guiRomList->setFillColor(Color::GrayLight);
    guiRomList->setOutlineColor(COL_ORANGE);
    renderer->add(guiRomList);

    // add rom info ui
    guiRomInfo = new GuiRomInfo(
            FloatRect(
                    (getLocalBounds().width / 2) + UI_MARGIN * scaling,
                    UI_MARGIN * scaling,
                    (getLocalBounds().width / 2) - UI_MARGIN * scaling * 2,
                    getLocalBounds().height - UI_MARGIN * scaling * 2), scaling);
    guiRomInfo->rectangle->setOutlineThickness(getOutlineThickness());
    guiRomInfo->update(roms[0]);
    renderer->add(guiRomInfo);
}

Gui::~Gui() {

    delete (menu_gui);
    delete (menu_rom);
}

void Gui::Run() {

    int rom_index = 0;
    int title_loaded = 0;

    Timer *timer_input = new Timer();
    Timer *timer_load = new Timer();

    UpdateInputMapping(false);

    while (!quit) {

        Input::Player *players = input->Update();

        int key = players[0].state;
        if (key > 0) {

            if (key & Input::Key::KEY_UP) {
                rom_index--;
                if (rom_index < 0)
                    rom_index = (int) (roms.size() - 1);
                guiRomList->setSelection(rom_index);
                guiRomInfo->update(NULL);
                title_loaded = 0;
            } else if (key & Input::Key::KEY_DOWN) {
                rom_index++;
                if (rom_index >= roms.size())
                    rom_index = 0;
                guiRomList->setSelection(rom_index);
                guiRomInfo->update(NULL);
                title_loaded = 0;
            } else if (key & Input::Key::KEY_RIGHT) {
                rom_index += guiRomList->getMaxLines();
                if (rom_index >= roms.size())
                    rom_index = (int) (roms.size() - 1);
                guiRomList->setSelection(rom_index);
                guiRomInfo->update(NULL);
                title_loaded = 0;
            } else if (key & Input::Key::KEY_LEFT) {
                rom_index -= guiRomList->getMaxLines();
                if (rom_index < 0)
                    rom_index = 0;
                guiRomList->setSelection(rom_index);
                guiRomInfo->update(NULL);
                title_loaded = 0;
            } else if (key & Input::Key::KEY_FIRE1) {
                if (romSelected != NULL
                    && romSelected->state != RomList::RomState::MISSING) {
                    // TODO
                    //RunRom(romSelected);
                }
            } else if (key & Input::Key::KEY_MENU1) {
                // TODO
                RunOptionMenu();
                //if (title != NULL) {
                // refresh preview/title image
                //}
            } else if (key & Input::Key::KEY_MENU2) {
                if (romSelected != NULL) {
                    config->Load(romSelected);
                    // TODO
                    RunOptionMenu(true);
                    //if (title != NULL) {
                    // refresh preview/title image
                    //}
                }
            } else if (key & EV_QUIT) {
                quit = true;
            }

            if (timer_input->GetSeconds() > 6) {
                renderer->delay(INPUT_DELAY / 5);
            } else if (timer_input->GetSeconds() > 2) {
                renderer->delay(INPUT_DELAY / 2);
            } else {
                renderer->delay(INPUT_DELAY);
            }
            timer_load->Reset();


        } else {

            if (/*romSelected != NULL &&*/ !title_loaded
                                           && timer_load->GetMillis() >= title_delay) {
                guiRomInfo->update(roms[rom_index]);
                /*
                if (TitleLoad(romSelected)) {
                    // refresh preview/title image
                }
                */
                title_loaded = 1;
                timer_load->Reset();
            }
            timer_input->Reset();
        }

        renderer->flip();
    }

    delete (timer_input);
    delete (timer_load);
}

void Gui::RunOptionMenu(bool isRomConfig) {

}

void Gui::RunStatesMenu() {

}

void Gui::FilterRoms() {

    roms.clear();

    int showClone = config->GetGuiValue(Option::Index::GUI_SHOW_CLONES);
    int showAll = config->GetGuiValue(Option::Index::GUI_SHOW_ALL);
    int showHardwareCfg = config->GetGuiValue(Option::Index::GUI_SHOW_HARDWARE);
    int showHardware = config->hardwareList[showHardwareCfg].prefix;

    remove_copy_if(romList->list.begin(), romList->list.end(), back_inserter(roms),
                   [showAll, showClone, showHardware](RomList::Rom *r) {
                       return !showAll && r->state != RomList::RomState::WORKING
                              || !showClone && r->parent != NULL
                              || showHardware != HARDWARE_PREFIX_ALL && !RomList::IsHardware(r->hardware, showHardware);
                   });

    // TODO
    //rom_index = 0;
}

void Gui::RunRom(RomList::Rom *rom) {

    if (rom == NULL) {
        return;
    }

    char path[MAX_PATH];
    for (int i = 0; i < DIRS_MAX; i++) {
        if (strlen(config->GetRomPath(i)) > 0) {
            sprintf(path, "%s%s.zip", config->GetRomPath(i), rom->zip);
            printf("%s\n", path);
            if (io->Exist(path))
                break;
        }
    }

    if (!io->Exist(path)) {
        printf("RunRom: rom not found: `%s`\n", rom->zip);
        return;
    }

    printf("RunRom: %s\n", path);
    for (nBurnDrvSelect[0] = 0; nBurnDrvSelect[0] < nBurnDrvCount; nBurnDrvSelect[0]++) {
        nBurnDrvActive = nBurnDrvSelect[0];
        if (strcasecmp(rom->zip, BurnDrvGetTextA(DRV_NAME)) == 0)
            break;
    }

    if (nBurnDrvActive >= nBurnDrvCount) {
        printf("RunRom: driver not found\n");
        return;
    }

    // load rom settings
    printf("RunRom: config->LoadRom(%s)\n", rom->zip);
    config->Load(rom);

    // set per rom input scheme
    UpdateInputMapping(true);

    printf("RunRom: RunEmulator: start\n");
    RunEmulator(this, nBurnDrvActive);

    // set default input scheme
    UpdateInputMapping(false);

    printf("RunRom: RunEmulator: return\n");
}

Input *Gui::GetInput() {
    return input;
}

Renderer *Gui::GetRenderer() {
    return renderer;
}

Skin *Gui::GetSkin() {
    return skin;
}

Config *Gui::GetConfig() {
    return config;
}

void Gui::SetTitleLoadDelay(int delay) {
    title_delay = delay;
}

void Gui::UpdateInputMapping(bool isRomConfig) {

    if (isRomConfig) {
        input->SetKeyboardMapping(config->GetRomPlayerInputKeys(0));
        int dz = 2000 + config->GetRomValue(Option::Index::JOY_DEADZONE) * 2000;
        for (int i = 0; i < PLAYER_COUNT; i++) {
            input->SetJoystickMapping(i, config->GetRomPlayerInputButtons(i), dz);
            input->players[i].lx.id = config->GetRomValue(Option::Index::JOY_AXIS_LX);
            input->players[i].ly.id = config->GetRomValue(Option::Index::JOY_AXIS_LY);
            input->players[i].rx.id = config->GetRomValue(Option::Index::JOY_AXIS_RX);
            input->players[i].ry.id = config->GetRomValue(Option::Index::JOY_AXIS_RY);
        }
    } else {
        input->SetKeyboardMapping(config->GetGuiPlayerInputKeys(0));
        int dz = 2000 + config->GetGuiValue(Option::Index::JOY_DEADZONE) * 2000;
        for (int i = 0; i < PLAYER_COUNT; i++) {
            input->SetJoystickMapping(i, config->GetGuiPlayerInputButtons(i), dz);
            input->players[i].lx.id = config->GetGuiValue(Option::Index::JOY_AXIS_LX);
            input->players[i].ly.id = config->GetGuiValue(Option::Index::JOY_AXIS_LY);
            input->players[i].rx.id = config->GetGuiValue(Option::Index::JOY_AXIS_RX);
            input->players[i].ry.id = config->GetGuiValue(Option::Index::JOY_AXIS_RY);
        }
    }
}

void Gui::Clear() {
}

void Gui::Flip() {
    renderer->flip();
}
