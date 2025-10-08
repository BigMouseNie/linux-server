#include "uri_parser.h"

#include <sstream>

std::string UriParser::Path() const {
  std::string path;
  for (auto seg = uri_.pathHead; seg; seg = seg->next) {
    if (seg != uri_.pathHead) path.append("/");
    if (seg->text.first) {
      std::string seg_str(seg->text.first,
                          seg->text.afterLast - seg->text.first);
      size_t pos = seg_str.find(";");
      if (pos == std::string::npos)
        path.append(seg_str);
      else
        path.append(seg_str, 0, pos);
    }
  }
  return path;
}

std::string UriParser::PathWithParams() const {
  std::string path;
  for (auto seg = uri_.pathHead; seg; seg = seg->next) {
    if (seg != uri_.pathHead) path.append("/");
    if (seg->text.first) {
      path.append(seg->text.first, seg->text.afterLast - seg->text.first);
    }
  }
  return path;
}

std::string UriParser::GetValByQuery(const std::string& key) const {
  auto it = query_map_.find(key);
  if (it == query_map_.end()) {
    return "";
  }
  return it->second;
}

int UriParser::Parser(const std::string& uri) {
  memset(&uri_, 0, sizeof(uri_));
  if (uriParseSingleUriA(&uri_, uri.c_str(), nullptr) != URI_SUCCESS) {
    return -1;
  }
  return 0;
}

void UriParser::ParserQuery() {
  std::string query_str = Query();
  std::istringstream stream(query_str);
  std::string pair;

  while (std::getline(stream, pair, '&')) {
    size_t eq_pos = pair.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = pair.substr(0, eq_pos);
      std::string value = pair.substr(eq_pos + 1);
      query_map_[key] = value;
    }
  }
}

void UriParser::ParserPath() {
  std::string path;
  for (auto seg = uri_.pathHead; seg; seg = seg->next) {
    if (seg->text.first) {
      ParseMatrixSegment(
          std::string(seg->text.first, seg->text.afterLast - seg->text.first));
    }
  }
}

void UriParser::ParseMatrixSegment(const std::string& segment) {
  size_t pos = segment.find(';');
  if (pos == std::string::npos) {
    path_params_kv_.emplace_back(segment, KV());
    path_params_.emplace_back(segment, "");
    return;
  }

  path_params_kv_.emplace_back(segment.substr(0, pos), KV());
  path_params_.emplace_back(segment.substr(0, pos), segment.substr(pos + 1));
  KV& matrix_params = path_params_kv_.back().second;
  size_t start = pos + 1;
  while (start < segment.size()) {
    size_t end = segment.find(';', start);
    if (end == std::string::npos) {
      size_t eq = segment.find("=", start);
      if (eq != std::string::npos)
        matrix_params[segment.substr(start, eq - start)] =
            segment.substr(eq + 1);
      else
        matrix_params[segment.substr(start)] = "";
      break;
    }

    std::string param = segment.substr(start, end - start);
    size_t eq = param.find('=');
    if (eq != std::string::npos)
      matrix_params[param.substr(0, eq)] = param.substr(eq + 1);
    else
      matrix_params[param.substr(0, eq)] = "";
    start = end + 1;
  }
}
