/*
  Copyright 2018 Equinor ASA.

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

#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

namespace Opm {

    void ActionContext::add(const std::string& func, const std::string& arg, double value) {
        this->values[func + ":" + arg] = value;
    }

    ActionContext::ActionContext(const SummaryState& summary_state_arg) :
        summary_state(summary_state_arg)
    {
        for (const auto& pair : TimeMap::eclipseMonthIndices())
            this->add(pair.first, pair.second);
    }

    void ActionContext::add(const std::string& func, double value) {
        this->values[func] = value;
    }


    double ActionContext::get(const std::string& func, const std::string& arg) const {
        return this->get(func + ":" + arg);
    }

    double ActionContext::get(const std::string& key) const {
        const auto& iter = this->values.find(key);
        if (iter != this->values.end())
            return iter->second;

        return this->summary_state.get(key);
    }


    std::vector<std::string> ActionContext::wells(const std::string& key) const {
        return this->summary_state.wells(key);
    }

}
