﻿#include <cmath>
#include <iostream>
#include <fstream>
#include <memory.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include "common.h"
#include "scene.h"
#include "utility.inl"
using namespace std;
CScene::CScene(int index)
    : _index(index),
      _max_column(pow(index, 2)),
      _cur_point({0, 0})
{
    init();
}

CScene::~CScene()
{
}

void CScene::show() const
{
    cls();

    printUnderline();

    for (int row = 0; row < _max_column; ++row)
    {
        CBlock block = _row_block[row];
        block.print();
        printUnderline(row);
    }
}

void CScene::printUnderline(int line_no) const
{
    string underline;

    for (int colunm = 0; colunm < 9; ++colunm)
    {
        if (_cur_point.y == line_no && _cur_point.x == colunm)
            underline += "--^-";
        else
            underline += "----";
    }
    underline += '-';

    for (int i = 0; i < 4; i++) {
        underline[i * 12] = ((line_no + 1) % 3 == 0) ? '+' : '|';
    }

    cout << underline.c_str() << endl;
}

void CScene::init()
{
    memset(_map, UNSELECTED, sizeof _map);

    // column_block 所有列
    for (int col = 0; col < _max_column; ++col)
    {
        CBlock column_block;

        for (int row = 0; row < _max_column; ++row)
        {
            column_block.push_back(_map + row * 9 + col);
        }
        _column_block[col] = column_block;
    }

    // row_block 所有行
    for (int row = 0; row < _max_column; ++row)
    {
        CBlock row_block;

        for (int col = 0; col < _max_column; ++col)
        {
            row_block.push_back(_map + row * 9 + col);
        }
        _row_block[row] = row_block;
    }

    // xy_block 所有九宫格, [行][列]
    for (int block_index = 0; block_index < _max_column; ++block_index)
    {
        CBlock xy_block;

        int xy_begin = block_index / 3 * 27 + block_index % 3 * 3;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                xy_block.push_back(_map + xy_begin + i * 9 + j);
            }
        }
        _xy_block[block_index / 3][block_index % 3] = xy_block;
    }

    return;
}

bool CScene::setCurValue(const int nCurValue, int &nLastValue)
{
    auto point = _map[_cur_point.x + _cur_point.y * 9];
    if (point.state == State::ERASED)
    {
        nLastValue = point.value;
        setValue(nCurValue);
        return true;
    }
    else
        return false;
}

void CScene::setValue(const point_t& p, const int value)
{
    _map[p.x + p.y * 9].value = value;
}

void CScene::setValue(const int value)
{
    auto p = _cur_point;
    this->setValue(p, value);
}

// 选择count个格子清空
void CScene::eraseRandomGrids(const int count)
{
    point_value_t p = {UNSELECTED, State::ERASED};

    vector<int> v(81);
    for (int i = 0; i < 81; ++i) {
        v[i] = i;
    }

    for (int i = 0; i < count; ++i) {
        int r = random(0, v.size() - 1);
        _map[v[r]] = p;
        v.erase(v.begin() + r);
    }
}

bool CScene::isComplete()
{
    // 任何一个block未被填满，则肯定未完成
    for (size_t i = 0; i < 81; ++i)
    {
        if (_map[i].value == UNSELECTED)
            return false;
    }

    // 同时block里的数字还要符合规则
    for (int row = 0; row < 9; ++row)
    {
        for (int col = 0; col < 9; ++col)
        {
            if (!_row_block[row].isValid() || 
                !_column_block[col].isValid() || 
                !_xy_block[row / 3][col / 3].isValid())
                return false;
        }
    }

    return true;
}

void CScene::save(const char *filename) {
    fstream fs;
    // TODO: check whether the file has existed
    fs.open(filename, fstream::in | fstream::out | fstream::app);

    // save _map
    for (int i = 0; i < 81; i++) {
        fs << _map[i].value << ' ' << static_cast<int>(_map[i].state) << endl;
    }

    // save _cur_point
    fs << _cur_point.x << ' ' << _cur_point.y << endl;

    // save _vCommand
    fs << _vCommand.size() << endl;
    for (CCommand command : _vCommand) {
        point_t point = command.getPoint();
        fs << point.x << ' ' << point.y << ' '
           << command.getPreValue() << ' '
           << command.getCurValue() << endl;
    }

    fs.close();
}

void CScene::load(const char *filename) {
    fstream fs;
    // TODO: check whether the file has existed
    fs.open(filename, fstream::in | fstream::out | fstream::app);

    // load _map
    for (int i = 0; i < 81; i++) {
        int tmpState;
        fs >> _map[i].value >> tmpState;
        _map[i].state = static_cast<State>(tmpState);
    }

    // load _cur_point
    fs >> _cur_point.x >> _cur_point.y;

    // load _vCommand
    int commandSize;
    fs >> commandSize;
    for (int i = 0; i < commandSize; i++) {
        point_t point;
        int preValue, curValue;
        fs >> point.x >> point.y >> preValue >> curValue;
        _vCommand.emplace_back(this, point, preValue, curValue);
    }
}

void CScene::play()
{
    show();

    char key = '\0';
    while (1)
    {
        key = _getch();
        if (key >= '0' && key <= '9')
        {
            CCommand oCommand(this);
            if (!oCommand.execute(key - '0'))
            {
                cout << "this number can't be modified." << endl;
            }
            else
            {
                _vCommand.push_back(move(oCommand));  // XXX: move without move constructor
                show();
                continue;
            }
        }

        switch (key)
        {
        case 0x1B: // ESC
        {       
            cout << "quit game ? [Y/N]" << endl;
            string strInput;
            cin >> strInput;
            if (strInput[0] == 'y' || strInput[0] == 'Y') {
                cout << "do you want to save the game progress ? [Y/N]" << endl;
                cin >> strInput;
                if (strInput[0] == 'y' || strInput[0] == 'Y') {
                    cout << "input path of the progress file: ";
                    cin >> strInput;
                    save(strInput.c_str());
                }
                exit(0);
            }
            else
            {
                cout << "continue." << endl;
                break;
            }
        }
        case 0x75: // u
        {
            if (_vCommand.empty())
                cout << "no more action to undo." << endl;
            else
            {
                CCommand &oCommand = _vCommand.back();
                oCommand.undo();
                _vCommand.pop_back();
                show();
            }
            break;
        }
        case 0x61: // a
            _cur_point.x = (_cur_point.x - 1) < 0 ? 0 : _cur_point.x - 1;
            show();
            break;
        case 0x64: // d
            _cur_point.x = (_cur_point.x + 1) > 8 ? 8 : _cur_point.x + 1;
            show();
            break;
        case 0x73: // s
            _cur_point.y = (_cur_point.y + 1) > 8 ? 8 : _cur_point.y + 1;
            show();
            break;
        case 0x77: // w
            _cur_point.y = (_cur_point.y - 1) < 0 ? 0 : _cur_point.y - 1;
            show();
            break;
        case 0x0D: // enter
            if (isComplete())
            {
                cout << "congratulation! you win!" << endl;
                getchar();
                exit(0);
            }
            else
            {
                cout << "sorry, not completed." << endl;
            }
            break;
        default:
            break;
        }
    }
}

// 一个场景可以多次被初始化
void CScene::generate()
{
    // XXX: pseudo random
    static char map_pattern[10][10] = {
        "ighcabfde",
        "cabfdeigh",
        "fdeighcab",
        "ghiabcdef",
        "abcdefghi",
        "defghiabc",
        "higbcaefd",
        "bcaefdhig",
        "efdhigbca"};

    vector<char> v = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};

    // 产生字母到数字的随机映射
    unordered_map<char, int> hash_map;
    for (int i = 1; i <= 9; ++i)
    {
        int r = random(0, v.size() - 1);
        hash_map[v[r]] = i;
        v.erase(v.begin() + r);
    }

    // 填入场景
    for (int row = 0; row < 9; ++row)
    {
        for (int col = 0; col < 9; ++col)
        {
            point_t point = {row, col};
            char key = map_pattern[row][col];
            setValue(point, hash_map[key]);
        }
    }

    assert(isComplete());

    return;
}

bool CScene::setPointValue(const point_t &stPoint, const int nValue)
{
    auto point = _map[stPoint.x + stPoint.y * 9];
    if (State::ERASED == point.state)
    {
        _cur_point = stPoint;
        setValue(nValue);
        return true;
    }
    else
        return false;
}

point_t CScene::getCurPoint()
{
    return _cur_point;
}