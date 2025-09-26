#include <string>
#include <vector>

void splitString(std::string str, std::vector<std::string> &result, char delimiter) {
    size_t index;
    while (true) {
        if (str.find(delimiter) == std::string::npos) {
            result.push_back(str);
            break;
        }
        index = str.find(delimiter);
        result.push_back(str.substr(0, index));
        str = str.substr(index + 1);
    }
}

std::string removeDublicateWhitespaces(std::string str) {
    if (str.length() <= 1) {
        return str;
    }
    for (int i = 0; i < (str.length() - 1); i++) {
        if (str[i] == ' ' && str[i + 1] == ' ') {
            str.erase(i, 1);
            i--;
        }
    }
    return str;
}

std::string removeWhitespaces(std::string str) {
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == ' ') {
            str.erase(i, 1);
            i--;
        }
    }
    return str;
}

std::string removeNonAlphaNum(std::string str) {
    std::string oldString = str;
    for (int i = 0; i < str.length(); i++) {
        if (std::isalnum(static_cast<unsigned char>(str[i])) == 0 && str[i] != ' ' && str[i] != '/') {
            str.erase(i, 1);
            i--;
        }
    }
    return str;
}

std::string getShape(std::string& name, bool extract) {
    std::string shapeChars = "0123456789x/ ";
    bool xActive, slashActive, whitespaceActive;
    for (int i = 0; i < name.length(); i++) {
        if (!isdigit(name[i])) {
            continue;
        }
        xActive = false;
        slashActive = false;
        whitespaceActive = false;
        int j = i + 1;
        for (; j < name.length(); j++) {
            char c = name[j];
            if (c == 'x') {
                if (xActive || slashActive) {
                    break;
                }
                xActive = true;
            }
            else if (c == '/') {
                if (xActive || slashActive) {
                    break;
                }
                slashActive = true;
            }
            else if (isdigit(c)) {
                xActive = false;
                slashActive = false;
            }
            else if (c == ' ') {
                whitespaceActive = true;
                continue;
            }
            else {
                break;
            }
        }
        std::string result = name.substr(i, j - i);
        if (result.find('x') != std::string::npos) {
            if (extract) {
                name.erase(i, j - i);
                if (name[name.length() - 1] == ' ') {
                    name.erase(name.length() - 1, 1);
                }
            }
            result = removeDublicateWhitespaces(result);
            for (int i = 1; i < result.length(); i++) {
                if (result[i] == 'x') {
                    if (result[i - 1] != ' ') {
                        result.insert(i, std::string{' '});
                        i++;
                    }
                    if (result[i + 1] != ' ') {
                        result.insert(i + 1, std::string{' '});
                        i++;
                    }
                }
            }
            if (result.back() == ' ') {
                result.pop_back();
            }
            return result;
        }
        i = j;
    }
    return std::string{""};
}


