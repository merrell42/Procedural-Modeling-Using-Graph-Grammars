#include "pch.h"
#include "svg_profile.h"
#include <cctype>
#include <fstream>
#include <stdexcept>

using namespace std;

namespace {

string joinPath(const string& directory, const string& file) {
    if (directory.empty()) {
        return file;
    }
    char last = directory.back();
    if (last == '/' || last == '\\') {
        return directory + file;
    }
    return directory + "/" + file;
}

bool isAbsolutePath(const string& path) {
    if (path.empty()) {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    return path.size() > 2 && path[1] == ':' && (path[2] == '/' || path[2] == '\\');
}

string svgPath(const string& name, const string& directory) {
    string file = name;
    string extension = file.size() >= 4 ? file.substr(file.size() - 4) : "";
    for (char& c : extension) {
        c = (char)tolower((unsigned char)c);
    }
    if (extension != ".svg") {
        file += ".svg";
    }
    if (isAbsolutePath(file)) {
        return file;
    }
    return joinPath(directory, file);
}

bool readSvgNumber(const string& text, size_t& index, double& value) {
    while (index < text.size() && (text[index] == ',' || isspace((unsigned char)text[index]))) {
        index++;
    }
    if (index >= text.size()) {
        return false;
    }
    size_t start = index;
    if (text[index] == '+' || text[index] == '-') {
        index++;
    }
    bool digit = false;
    while (index < text.size() && isdigit((unsigned char)text[index])) {
        digit = true;
        index++;
    }
    if (index < text.size() && text[index] == '.') {
        index++;
        while (index < text.size() && isdigit((unsigned char)text[index])) {
            digit = true;
            index++;
        }
    }
    if (!digit) {
        return false;
    }
    value = stod(text.substr(start, index - start));
    return true;
}

vector<Vec2> parseSvgPoints(const string& points) {
    vector<Vec2> polygon;
    size_t index = 0;
    while (index < points.size()) {
        double x = 0;
        double y = 0;
        if (!readSvgNumber(points, index, x)) {
            break;
        }
        if (!readSvgNumber(points, index, y)) {
            throw runtime_error("SVG polygon points are not in x,y pairs");
        }
        polygon.emplace_back(x, y);
    }
    return polygon;
}

string svgAttribute(const string& content, const string& tag, const string& attribute) {
    string lower = content;
    for (char& c : lower) {
        c = (char)tolower((unsigned char)c);
    }
    size_t tagAt = lower.find("<" + tag);
    if (tagAt == string::npos) {
        return "";
    }
    size_t tagEnd = lower.find('>', tagAt);
    if (tagEnd == string::npos) {
        return "";
    }
    string tagText = lower.substr(tagAt, tagEnd - tagAt);
    size_t nameAt = tagText.find(attribute);
    if (nameAt == string::npos) {
        return "";
    }
    size_t equals = tagText.find('=', nameAt + attribute.size());
    if (equals == string::npos) {
        return "";
    }
    size_t quote = tagText.find_first_of("\"'", equals + 1);
    if (quote == string::npos) {
        return "";
    }
    char mark = tagText[quote];
    size_t valueStart = quote + 1;
    size_t valueEnd = tagText.find(mark, valueStart);
    if (valueEnd == string::npos) {
        return "";
    }
    return content.substr(tagAt + valueStart, valueEnd - valueStart);
}

}

vector<Vec2> loadSvgProfile(const string& name, const string& directory) {
    string path = svgPath(name, directory);
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Could not open SVG profile: " + path);
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    string points = svgAttribute(content, "polygon", "points");
    if (points.empty()) {
        points = svgAttribute(content, "polyline", "points");
    }
    if (points.empty()) {
        throw runtime_error("SVG profile has no polygon points: " + path);
    }
    vector<Vec2> polygon = parseSvgPoints(points);
    if (polygon.size() < 3) {
        throw runtime_error("SVG profile needs at least 3 points: " + path);
    }
    return polygon;
}
