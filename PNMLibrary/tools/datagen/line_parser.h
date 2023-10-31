
/*
 * Copyright (C) 2022 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef SLS_LINE_PARSER_H
#define SLS_LINE_PARSER_H

#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace tools::gen {

/*! \brief Line parser for ini file entry. */
class LineParser {
public:
  using parse_closure = std::function<void(std::istringstream &)>;

  template <typename Closure>
  void register_parse_closure(std::string name, Closure func) {
    function_set_.emplace(std::move(name), std::move(func));
  }

  void parse_line(const std::string &line) {
    std::istringstream iss(line);
    std::string var_name;
    iss >> var_name;
    char tmp;
    iss >> tmp;
    function_set_.at(var_name)(iss);
  }

  void parse_file(std::ifstream &in) {
    std::string var_line;
    while (std::getline(in, var_line)) {
      if (!var_line.empty()) {
        parse_line(var_line);
      }
    }
  }

private:
  std::unordered_map<std::string, parse_closure> function_set_;
};

} // namespace tools::gen

#endif // SLS_LINE_PARSER_H
