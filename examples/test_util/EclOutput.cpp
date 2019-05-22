/*
   Copyright 2019 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include "EclOutput.hpp"
#include "EclUtil.hpp"
#include <opm/common/ErrorMacros.hpp>

#include <algorithm>
#include <iterator>
#include <iomanip>
#include <stdexcept>
#include <typeinfo>
#include <sstream>
#include <stdio.h>


EclOutput::EclOutput(const std::string& inputFile, bool formatted) :
  isFormatted(formatted)
{
    ofileH.open(inputFile, isFormatted ? std::ios::out : std::ios::out | std::ios::binary);
}


template<>
void EclOutput::write<std::string>(const std::string& name,
                                   const std::vector<std::string>& data)
{
    if (isFormatted)
    {
        writeFormattedHeader(name, data.size(), EIOD::CHAR);
        writeFormattedCharArray(data);
    }
    else
    {
        writeBinaryHeader(name, data.size(), EIOD::CHAR);
        writeBinaryCharArray(data);
    }
}


void EclOutput::writeBinaryHeader(const std::string&arrName, int size, EIOD::eclArrType arrType)
{
    std::string name = arrName + std::string(8 - arrName.size(),' ');

    int flippedSize = EIOD::flipEndianInt(size);
    int bhead = EIOD::flipEndianInt(16);

    ofileH.write(reinterpret_cast<char*>(&bhead), sizeof(bhead));

    ofileH.write(name.c_str(), 8);
    ofileH.write(reinterpret_cast<char*>(&flippedSize), sizeof(flippedSize));

    switch(arrType) {
    case EIOD::INTE:
        ofileH.write("INTE", 4);
        break;
    case EIOD::REAL:
        ofileH.write("REAL", 4);
        break;
    case EIOD::DOUB:
        ofileH.write("DOUB", 4);
        break;
    case EIOD::LOGI:
        ofileH.write("LOGI", 4);
        break;
    case EIOD::CHAR:
        ofileH.write("CHAR", 4);
        break;
    case EIOD::MESS:
        ofileH.write("MESS", 4);
        break;
    }

    ofileH.write(reinterpret_cast<char *>(&bhead), sizeof(bhead));
}


template <typename T>
void EclOutput::writeBinaryArray(const std::vector<T>& data)
{
    int rest,num,rval;
    int dhead;
    float value_f;
    double value_d;
    int intVal;

    int n = 0;
    int size = data.size();

    EIOD::eclArrType arrType = EIOD::MESS;

    if (typeid(std::vector<T>) == typeid(std::vector<int>)) {
        arrType = EIOD::INTE;
    } else if (typeid(std::vector<T>) == typeid(std::vector<float>)) {
        arrType = EIOD::REAL;
    } else if (typeid(std::vector<T>) == typeid(std::vector<double>)) {
        arrType = EIOD::DOUB;
    } else if (typeid(std::vector<T>) == typeid(std::vector<bool>)) {
        arrType = EIOD::LOGI;
    }

    auto sizeData = block_size_data_binary(arrType);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements = maxBlockSize / sizeOfElement;

    if (!ofileH.is_open()) {
        OPM_THROW(std::runtime_error, "fstream fileH not open for writing");
    }

    rest = size * sizeOfElement;
    while (rest > 0) {
        if (rest > maxBlockSize) {
            rest -= maxBlockSize;
            num = maxNumberOfElements;
        } else {
            num = rest / sizeOfElement;
            rest = 0;
        }

        dhead = EIOD::flipEndianInt(num * sizeOfElement);

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));

        for (int i = 0; i < num; i++) {
            if (arrType == EIOD::INTE) {
                rval = EIOD::flipEndianInt(data[n]);
                ofileH.write(reinterpret_cast<char*>(&rval), sizeof(rval));
            } else if (arrType == EIOD::REAL) {
                value_f = EIOD::flipEndianFloat(data[n]);
                ofileH.write(reinterpret_cast<char*>(&value_f), sizeof(value_f));
            } else if (arrType == EIOD::DOUB) {
                value_d = EIOD::flipEndianDouble(data[n]);
                ofileH.write(reinterpret_cast<char*>(&value_d), sizeof(value_d));
            } else if (arrType == EIOD::LOGI) {
                intVal = data[n] ? EIOD::true_value : EIOD::false_value;
                ofileH.write(reinterpret_cast<char*>(&intVal), sizeOfElement);
            } else {
                std::cout << "type not supported in write binaryarray" << std::endl;
                exit(1);
            }

            n++;
        }

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));
    }
}


template void EclOutput::writeBinaryArray<int>(const std::vector<int>& data);
template void EclOutput::writeBinaryArray<float>(const std::vector<float>& data);
template void EclOutput::writeBinaryArray<double>(const std::vector<double>& data);
template void EclOutput::writeBinaryArray<bool>(const std::vector<bool>& data);
template void EclOutput::writeBinaryArray<char>(const std::vector<char>& data);


void EclOutput::writeBinaryCharArray(const std::vector<std::string>& data)
{
    int num,dhead;

    int n = 0;
    int size = data.size();

    auto sizeData = EIOD::block_size_data_binary(EIOD::CHAR);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements = maxBlockSize / sizeOfElement;

    int rest = size * sizeOfElement;

    if (!ofileH.is_open()) {
        OPM_THROW(std::runtime_error,"fstream fileH not open for writing");
    }

    while (rest > 0) {
        if (rest > maxBlockSize) {
            rest -= maxBlockSize;
            num = maxNumberOfElements;
        } else {
            num = rest / sizeOfElement;
            rest = 0;
        }

        dhead = EIOD::flipEndianInt(num * sizeOfElement);

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));

        for (int i = 0; i < num; i++) {
            std::string tmpStr = data[n] + std::string(8 - data[n].size(),' ');
            ofileH.write(tmpStr.c_str(), sizeOfElement);
            n++;
        }

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));
    }
}


void EclOutput::writeFormattedHeader(const std::string& arrName, int size, EIOD::eclArrType arrType)
{
    std::string name = arrName + std::string(8 - arrName.size(),' ');

    ofileH << " '" << name << "' " << std::setw(11) << size;

    switch (arrType) {
    case EIOD::INTE:
        ofileH << " 'INTE'" <<  std::endl;
        break;
    case EIOD::REAL:
        ofileH << " 'REAL'" <<  std::endl;
        break;
    case EIOD::DOUB:
        ofileH << " 'DOUB'" <<  std::endl;
        break;
    case EIOD::LOGI:
        ofileH << " 'LOGI'" <<  std::endl;
        break;
    case EIOD::CHAR:
        ofileH << " 'CHAR'" <<  std::endl;
        break;
    case EIOD::MESS:
        ofileH << " 'MESS'" <<  std::endl;
        break;
    }
}


std::string EclOutput::make_real_string(float value) const
{
    char buffer [15];
    sprintf (buffer, "%10.7E", value);

    if (value == 0.0) {
        return "0.00000000E+00";
    } else {
        std::string tmpstr(buffer);

        int exp =  value < 0.0 ? std::stoi(tmpstr.substr(11, 3)) :  std::stoi(tmpstr.substr(10, 3));

        if (value < 0.0) {
            tmpstr = "-0." + tmpstr.substr(1, 1) + tmpstr.substr(3, 7) + "E";
        } else {
            tmpstr = "0." + tmpstr.substr(0, 1) + tmpstr.substr(2, 7) +"E";
        }

        sprintf (buffer, "%+03i", exp+1);
        tmpstr = tmpstr+buffer;

        return tmpstr;
    }
}


std::string EclOutput::make_doub_string(double value) const
{
    char buffer [21];
    sprintf (buffer, "%19.13E", value);

    if (value == 0.0) {
        return "0.00000000000000D+00";
    } else {
        std::string tmpstr(buffer);

        int exp =  value < 0.0 ? std::stoi(tmpstr.substr(17, 4)) :  std::stoi(tmpstr.substr(16, 4));

        if (value < 0.0) {
            if (abs(exp) < 100) {
                tmpstr = "-0." + tmpstr.substr(1, 1) + tmpstr.substr(3, 13) + "D";
            } else {
                tmpstr = "-0." + tmpstr.substr(1, 1) + tmpstr.substr(3, 13);
            }
        } else {
            if (abs(exp) < 100) {
                tmpstr = "0." + tmpstr.substr(0, 1) + tmpstr.substr(2, 13) + "D";
            } else {
                tmpstr = "0." + tmpstr.substr(0, 1) + tmpstr.substr(2, 13);
            }
        }

        sprintf (buffer, "%+03i", exp+1);
        tmpstr = tmpstr + buffer;

        return tmpstr;
    }
}


template <typename T>
void EclOutput::writeFormattedArray(const std::vector<T>& data)
{
    int size = data.size();
    int n = 0;

    EIOD::eclArrType arrType = EIOD::MESS;
    if (typeid(T) == typeid(int)) {
        arrType = EIOD::INTE;
    } else if (typeid(T) == typeid(float)) {
        arrType = EIOD::REAL;
    } else if (typeid(T) == typeid(double)) {
        arrType = EIOD::DOUB;
    } else if (typeid(T) == typeid(bool)) {
        arrType = EIOD::LOGI;
    }

    auto sizeData = EIOD::block_size_data_formatted(arrType);

    int maxBlockSize = std::get<0>(sizeData);
    int nColumns = std::get<1>(sizeData);
    int columnWidth = std::get<2>(sizeData);

    for (int i = 0; i < size; i++) {
        n++;

        switch (arrType) {
        case EIOD::INTE:
            ofileH << std::setw(columnWidth) << data[i];
            break;
        case EIOD::REAL:
            ofileH << std::setw(columnWidth) << make_real_string(data[i]);
            break;
        case EIOD::DOUB:
            ofileH << std::setw(columnWidth) << make_doub_string(data[i]);
            break;
        case EIOD::LOGI:
            if (data[i]) {
                ofileH << "  T";
            } else {
                ofileH << "  F";
            }
            break;
        default:
            break;
        }

        if ((n % nColumns) == 0 || (n % maxBlockSize) == 0) {
            ofileH << std::endl;
        }

        if ((n % maxBlockSize) == 0) {
            n=0;
        }
    }

    if ((n % nColumns) != 0 && (n % maxBlockSize) != 0) {
        ofileH << std::endl;
    }
}


template void EclOutput::writeFormattedArray<int>(const std::vector<int>& data);
template void EclOutput::writeFormattedArray<float>(const std::vector<float>& data);
template void EclOutput::writeFormattedArray<double>(const std::vector<double>& data);
template void EclOutput::writeFormattedArray<bool>(const std::vector<bool>& data);
template void EclOutput::writeFormattedArray<char>(const std::vector<char>& data);


void EclOutput::writeFormattedCharArray(const std::vector<std::string>& data)
{
    auto sizeData = EIOD::block_size_data_formatted(EIOD::CHAR);

    int nColumns = std::get<1>(sizeData);

    int size = data.size();

    for (int i = 0; i < size; i++) {
        std::string str1(8,' ');
        str1 = data[i] + std::string(8 - data[i].size(),' ');

        ofileH << " '" << str1 << "'";

        if ((i+1) % nColumns == 0) {
            ofileH  << std::endl;
        }
    }

    if ((size % nColumns) != 0) {
        ofileH  << std::endl;
    }
}
