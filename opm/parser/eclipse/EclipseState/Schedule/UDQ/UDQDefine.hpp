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


#ifndef UDQ_DEFINE_HPP
#define UDQ_DEFINE_HPP

#include <string>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQWellSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>

namespace Opm {

class UDQASTNode;
class ParseContext;
class ErrorGuard;

class UDQDefine{
public:
    UDQDefine(const UDQParams& udq_params,
              const std::string& keyword,
              const std::vector<std::string>& deck_data);

    UDQDefine(const UDQParams& udq_params,
              const std::string& keyword,
              const std::vector<std::string>& deck_data,
              const ParseContext& parseContext,
              ErrorGuard& errors);

    template <typename T>
    UDQDefine(const UDQParams& udq_params,
              const std::string& keyword,
              const std::vector<std::string>& deck_data,
              const ParseContext& parseContext,
              T&& errors);


    UDQWellSet eval_wells(const UDQContext& context) const;
    UDQSet eval(const UDQContext& context) const;
    const std::string& keyword() const;
    const std::vector<std::string>& tokens() const;
    UDQVarType  var_type() const;
private:
    std::vector<std::string> input_tokens;
    const UDQParams& udq_params;  // Beacuse of the shared RNG stream this must be a reference.
    std::string m_keyword;
    std::shared_ptr<UDQASTNode> ast;
    UDQVarType m_var_type;
};
}



#endif
