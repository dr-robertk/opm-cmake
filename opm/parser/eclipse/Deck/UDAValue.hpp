/*
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UDA_VALUE_HPP
#define UDA_VALUE_HPP

#include <stdexcept>
#include <vector>
#include <string>

#include <opm/parser/eclipse/Units/Dimension.hpp>

namespace Opm {

class UDAValue {
public:
    UDAValue();
    explicit UDAValue(double);
    explicit UDAValue(const std::string&);

    template<typename T>
    T get() const;

    template<typename T>
    bool is() const;

    void set_dim(const Dimension& dim) const;
    const Dimension& get_dim() const;

    bool operator==(const UDAValue& other) const;
    bool operator!=(const UDAValue& other) const;
private:
    bool numeric_value;
    double double_value;
    std::string string_value;

    /* This 'mutable' modifier is a hack to avoid tampering with the overall
       const-ness of the data in a deck item. */
    mutable Dimension dim;
};
}



#endif
