#include <HID.h>
#include <Time.h>
#include "CommandExecutor.h"

template<class Serial_T>
CommandExecutor<Serial_T>::CommandExecutor(Serial_T *serial, LiquidCrystal_I2C *lcd, DS1307RTC *rtc, Beeper *beeper,
                                           Light *light, Player *player) {
    this->serial = serial;
    this->lcd = lcd;
    this->rtc = rtc;
    this->beeper = beeper;
    this->light = light;
    this->player = player;
}

boolean isStrDigit(char *str) {
    if (str == NULL)
        return false;
    for (int i = 0; str[i] != 0; i++) {
        if (!isDigit(str[i]))
            return false;
    }
    return true;
}

char *cmdUnit;
int unitsCnt = 0, readUnitsCnt = 0;

void splitStr(char *str, char token) {
    for (int i = 0; str[i] != 0; i++) {
        if (str[i] == token) {
            str[i] = 0;
            unitsCnt++;
        }
    }
    unitsCnt++;
    readUnitsCnt = 0;
    cmdUnit = str;
}

void nextCmdUnit() {
    if (readUnitsCnt < unitsCnt) {
        for (; *cmdUnit != 0; cmdUnit++) {}
        cmdUnit++;
        readUnitsCnt++;
    } else cmdUnit = NULL;
}

boolean cmdEq(const char *cmd) {
    return strcmp(cmdUnit, cmd) == 0;
}

boolean readInt(int &buf) {
    nextCmdUnit();
    if (isStrDigit(cmdUnit)) {
        buf = atoi(cmdUnit);
        return true;
    }
    return false;
}

template<class Serial_T>
void CommandExecutor<Serial_T>::execute(char *cmd) {
    beeper->beep(1000, 50);
    splitStr(cmd, ' ');

    if (cmdEq("time")) {
        nextCmdUnit();
        if (cmdEq("set")) {
            int year, month, day, hour, min, sec;
            if (readInt(year) && readInt(month) && readInt(day) &&
                readInt(hour) && readInt(min) && readInt(sec)) {
                tmElements_t tm;
                tm.Year = (uint8_t) year;
                tm.Month = (uint8_t) month;
                tm.Day = (uint8_t) day;
                tm.Hour = (uint8_t) hour;
                tm.Minute = (uint8_t) min;
                tm.Second = (uint8_t) sec;
                rtc->write(tm);
                serial->println("OK");
            } else serial->println("Error: digit expected");
        }
        if (cmdEq("get")) {
            tmElements_t tm;
            rtc->read(tm);
            char *str = new char[30];
            sprintf(str, "time: %d %d %d %d %d %d", tm.Year, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
            serial->println(str);
            delete[] str;
        }
    } else if (cmdEq("light")) {
        nextCmdUnit();
        if (cmdEq("set")) {
            nextCmdUnit();
            if (cmdEq("on")) {
                light->turnOn();
                serial->println("OK");
            }
            if (cmdEq("off")) {
                light->turnOff();
                serial->println("OK");
            }
        }
        if (cmdEq("get")) {
            serial->println(light->getCondition());
        }
    } else if (cmdEq("beep")) {
        beeper->beep();
        serial->println("OK");
    } else if (cmdEq("player")) {
        nextCmdUnit();
        if (cmdEq("play")) {
            player->resetSong();
            player->start();
            serial->println("OK");
        }
        if (cmdEq("get")) {
            for (int i = 0; i < player->songsLength; i++) {
                serial->print(i);
                serial->print(":");
                for (int j = 0; player->songs[i][j] != ':'; j++) {
                    serial->print(player->songs[i][j]);
                }
                serial->println();
            }
        }
        if (cmdEq("set")) {
            int idx;
            if (readInt(idx)) {
                player->setSong((char *) player->songs[idx % player->songsLength]);
                serial->println("OK");
            } else serial->println("Error: digit expected");
        }
        if (cmdEq("repeat")) {
            nextCmdUnit();
            if (cmdEq("get")) {
                serial->println(player->getRepeat());
            }
            if (cmdEq("set")) {
                nextCmdUnit();
                if (cmdEq("on")) {
                    player->setRepeat(true);
                    serial->println("OK");
                }
                if (cmdEq("off")) {
                    player->setRepeat(false);
                    serial->println("OK");
                }
            }
        }

    } else if (cmdEq("alert")) {
        nextCmdUnit();
        if (cmdUnit != NULL) {
            lcd->clear();
            lcd->print(cmdUnit);
            beeper->beep(1000, 1000);
            delay(2000);
            lcd->clear();
        }
    } else {
        serial->println("Cannot execute");
    }
}

template
class CommandExecutor<SoftwareSerial>;

template
class CommandExecutor<HardwareSerial>;