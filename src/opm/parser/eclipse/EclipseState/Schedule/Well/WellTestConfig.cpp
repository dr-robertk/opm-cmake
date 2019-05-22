/*
  Copyright 2018 Statoil ASA.

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
#include <stdexcept>
#include <algorithm>

#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>

namespace Opm {


WellTestConfig::WellTestConfig() {

}

void WellTestConfig::add_well(const std::string& well, Reason shut_reason, double test_interval, int num_retries, double startup_time) {
    wells.push_back({well, shut_reason, test_interval, num_retries, startup_time});
}


void WellTestConfig::add_well(const std::string& well, const std::string& reasons, double test_interval, int num_retries, double startup_time) {
    if (reasons.size() == 0)
        throw std::invalid_argument("Can not pass empty string to stop testing to add_well() method.");

    for (auto c : reasons) {
        switch(c) {
        case 'P' :
            add_well(well, Reason::PHYSICAL, test_interval, num_retries, startup_time);
            break;
         case 'E' :
            add_well(well, Reason::ECONOMIC, test_interval, num_retries, startup_time);
            break;
        case 'G':
            add_well(well, Reason::GROUP, test_interval, num_retries, startup_time);
            break;
        case 'D':
            add_well(well, Reason::THP_DESIGN, test_interval, num_retries, startup_time);
            break;
        case 'C':
            add_well(well, Reason::COMPLETION, test_interval, num_retries, startup_time);
            break;
        default:
            throw std::invalid_argument("Invalid character in WTEST configuration");
        }
    }
}


void WellTestConfig::drop_well(const std::string& well) {
    wells.erase(std::remove_if(wells.begin(),
                               wells.end(),
                               [&well](const WTESTWell& wtest_well) { return (wtest_well.name == well); }),
                wells.end());
}

bool WellTestConfig::has(const std::string& well) const {
    const auto well_iter = std::find_if(wells.begin(),
                                        wells.end(),
                                        [&well](const WTESTWell& wtest_well) { return (wtest_well.name == well); });
    return (well_iter != wells.end());
}


bool WellTestConfig::has(const std::string& well, Reason reason) const {
    const auto well_iter = std::find_if(wells.begin(),
                                        wells.end(),
                                        [&well, &reason](const WTESTWell& wtest_well)
                                        {
                                            return (reason == wtest_well.shut_reason && wtest_well.name == well);
                                        });
    return (well_iter != wells.end());
}


const WellTestConfig::WTESTWell& WellTestConfig::get(const std::string& well, Reason reason) const {
    const auto well_iter = std::find_if(wells.begin(),
                                        wells.end(),
                                        [&well, &reason](const WTESTWell& wtest_well)
                                        {
                                            return (reason == wtest_well.shut_reason && wtest_well.name == well);
                                        });
    if (well_iter == wells.end())
        throw std::invalid_argument("No such WTEST object");

    return *well_iter;
}





size_t WellTestConfig::size() const {
    return wells.size();
}

}


