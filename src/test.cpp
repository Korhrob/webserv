#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <map>
#include <vector>

struct jsonValue {
    std::variant<std::string, double, bool, std::nullptr_t, 
    std::map<std::string, jsonValue>, 
    std::vector<jsonValue>> value;
};

using jsonMap = std::unordered_map<std::string, jsonValue>;

// jsonValue    parseValue(std::string& json, size_t& pos) {
//     // std::cout << "pos: " << pos << "\n";
//     // std::cout << json[pos] << "\n";
//     jsonValue val;
//     size_t delim = json.find('\n', pos);
//     if (delim == std::string::npos) {
//         val.value = "no nl";
//         return val;
//     }
//     std::string value = json.substr(pos, delim - pos);
//     if (value.back() == ',')
//         value.pop_back();
//     pos = delim;
//     // std::cout << value << "\n";
//     if (value == "true" || value == "false") {
//         val.value = value == "true" ? true : false;
//         return val;
//     }
//     if (value == "nullptr") {
//         val.value = nullptr;
//         return val;
//     }
//     if (value.front() == '\'' && value.back() == '\'' ) {
//         value.erase(0, 1);
//         value.pop_back();
//         val.value = value;
//         return val;
//     }
//     try {
//         size_t idx;
//         double numValue = std::stod(value, &idx);
//         if (idx == value.length()) {
//             val.value = numValue;
//             return val;
//         }
//     } catch (std::exception& e) {
//         std::cout << e.what() << "\n";
//     }
//     // std::cout << "delim: " << json[delim] << " " << delim << "\n";
//     val.value = "object or array";
//     return val;
// }

jsonValue	parseValue(std::string& body, size_t& pos)
{
	size_t	end = body.find('\n', pos);
	// if (end == std::string::npos) {}
		// throw HttpException::badRequest();

	std::string	str = body.substr(pos, end - pos);
	if (str.back() == ',')
		str.pop_back();

	jsonValue	val;
	// std::cout << "pos: " << pos << " end: " << end << "\n";
	// std::cout << "(value) pos in body: " << body[pos] << "\n";
	// std::cout << "value string: " << str << "\n";
	try {
		size_t	idx;
		double	numVal = std::stod(str, &idx);
		if (idx == str.length()) {
			val.value = numVal;
			pos = end;
			return val;
		}
	} catch (std::exception& e) {}
	
	if (str == "null")
		val.value = nullptr;
	else if (str == "true" || str == "false") 
		val.value = str == "true" ? true : false;
	else if (str.front() == '\'' && str.back() == '\'') 
		val.value = str.substr(1, str.length() - 2);
	else if (str.front() == '[') {
		val.value = str; // temp
	}
	else if (str.front() == '{') {
		val.value = str; // temp
	}
	// else
	// 	throw HttpException::badRequest();

	pos = end;

	return val;
}

std::string parseKey(std::string& json, size_t& pos) {
	// std::cout << "(key) pos in body: " << json[pos] << "\n";
    size_t  delim;
    delim = json.find(':', pos);
    if (delim == std::string::npos)
        return "no : found";

    // std::cout << "pos: " << pos << "delim: " << delim << "\n";
    if (json[pos] != '\'' || json[delim - 1] != '\'')
        return "no double quotes";

    std::string ret = json.substr(pos + 1, delim - pos - 2);
    pos = delim + 1;
    return ret;
}

void printjsonValue(const jsonValue& jValue) {
    std::visit([](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::string>) {
            std::cout << val;
        } else if constexpr (std::is_same_v<T, double>) {
            std::cout << val;
        } else if constexpr (std::is_same_v<T, bool>) {
            std::cout << (val ? "true" : "false");
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            std::cout << "null";
        } else if constexpr (std::is_same_v<T, std::map<std::string, jsonValue>>) {
            std::cout << "{ ";
            for (const auto& [key, value] : val) {
                std::cout << "\"" << key << "\": ";
                printjsonValue(value);
                std::cout << ", ";
            }
            std::cout << " }";
        } else if constexpr (std::is_same_v<T, std::vector<jsonValue>>) {
            std::cout << "[ ";
            for (const auto& element : val) {
                printjsonValue(element);
                std::cout << ", ";
            }
            std::cout << " ]";
        }
    }, jValue.value);
}

void printMap(const std::unordered_map<std::string, jsonValue>& jsonMap) {
    for (const auto& [key, jsonValue] : jsonMap) {
        std::cout << key << ": ";
        printjsonValue(jsonValue);
        std::cout << "\n";
    }
}

int main()
{
	jsonMap             m_jsonData;
	std::string	body("{\n'name': 'John',\n'age': 30,\n'isStudent': false,\n'grades': [85, 90, 95],\n'profile': { 'active': true, 'score': 100.5 }\n}");

	// if (body.front() != '{' && body.front() != '[')
	// 	throw HttpException::badRequest();

	std::cout << body << "\n";

	char	closingBracket = body.front() == '{' ? '}' : ']';
	size_t	pos = 1;
	while (std::isspace(body[pos]))
				pos++;
	while (body[pos] != closingBracket) {
		std::string key = parseKey(body, pos);
		while (std::isspace(body[pos]))
			pos++;
		jsonValue	value = parseValue(body, pos);
		while (std::isspace(body[pos]))
			pos++;
		m_jsonData[key] = value;
	}
	printMap(m_jsonData);
}

// int main() {
//     std::string         jsonStr("{\n'name': 'John',\n'age': 30,\n'isStudent': false,\n'grades': [85, 90, 95],\n'profile': { 'active': true, 'score': 100.5 }\n}");
//     std::istringstream  json(jsonStr);
//     jsonMap             m_jsonData;
//     char                bracket;
//     size_t              pos = 1;
    
//     std::cout << jsonStr << "\n";
//     if (jsonStr.front() == '{' || jsonStr.front() == '[')
//         bracket = jsonStr[0] == '{' ? '}' : ']';

//     while (jsonStr[pos] < jsonStr.length()) {
//         while (std::isspace(jsonStr[pos])) ++pos;

//         std::string key = parseKey(jsonStr, pos);
// 		std::cout << "parsed key\n";

//         while (std::isspace(jsonStr[pos])) ++pos;

//         jsonValue value = parseValue(jsonStr, pos);
// 		std::cout << "parsed value\n";
//         if (jsonStr[pos] == ',') ++pos;

//         m_jsonData[key] = value;
//     }
//     printMap(m_jsonData);
// }
