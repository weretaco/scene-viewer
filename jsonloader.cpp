#include "jsonloader.h"

#include <iostream>
#include <string>
#include <cctype>

JsonLoader::JsonLoader(std::string fileName) {
    fileStream.open(fileName, std::ios::in);
    root = nullptr;
    prevPos = 0;

    if (fileStream.fail()) {
        throw std::runtime_error("Failed to open scene file: " + fileName);
    }
}

JsonLoader::JsonNode* JsonLoader::parseJson() {
    while (!fileStream.eof()) {
        try {
            JsonNode* node = parseNode();

            if (!root) {
                root = node;
            }
        } catch (const std::logic_error&) {
            break;
        }
    }

    return root;
}

JsonLoader::JsonNode* JsonLoader::parseObject() {
    JsonNode* node = new JsonNode();
    node->type = JsonNode::Type::OBJECT;

    std::map<std::string, JsonNode*>* obj = new std::map<std::string, JsonNode*>();

    bool completed = false;
    while (!completed) {
        if (fileStream.eof()) {
            throw std::logic_error("No more tokens");
        } else {
            Token token = getToken();
            if (token.type == Token::Type::CURLY_CLOSE) { // check for an empty object
                completed = true;
                continue;
            }
            std::string key = token.value;

            getToken(); // get the colon

            (*obj)[key] = parseNode();

            token = getToken(); // get the comma, unless it's the element in the object
            if (token.type == Token::Type::CURLY_CLOSE) {
                completed = true;
            }
        }
    }
    node->value = obj;

    return node;
}

JsonLoader::JsonNode* JsonLoader::parseArray() {
    JsonNode* node = new JsonNode();
    node->type = JsonNode::Type::ARRAY;

    std::vector<JsonNode*>* arr = new std::vector<JsonNode*>();

    bool completed = false;
    while (!completed) {
        if (fileStream.eof()) {
            throw std::logic_error("No more tokens");
        } else {
            arr->push_back(parseNode());

            Token token = getToken();
            if (token.type == Token::Type::ARRAY_CLOSE) {
                completed = true;
            }
        }
    }
    node->value = arr;

    return node;
}

JsonLoader::JsonNode* JsonLoader::parseString() {
    JsonNode* node = new JsonNode();
    node->type = JsonNode::Type::STRING;

    Token token = getToken();
    node->value = token.value;

    return node;
}

JsonLoader::JsonNode* JsonLoader::parseNumber() {
    JsonNode* node = new JsonNode();
    node->type = JsonNode::Type::NUMBER;

    Token token = getToken();
    node->value = std::stof(token.value);

    return node;
}

JsonLoader::JsonNode* JsonLoader::parseNode() {
    JsonNode* node;

    Token token = getToken();

    switch (token.type) {
        case Token::Type::CURLY_OPEN: {
            node = parseObject();
            break;
        }
        case Token::Type::ARRAY_OPEN: {
            node = parseArray();
            break;
        }
        case Token::Type::STRING: {
            rollbackToken();
            node = parseString();
            break;
        }
        case Token::Type::NUMBER: {
            rollbackToken();
            node = parseNumber();
            break;
        default:
            std::cout << "TOKEN VALUE: " << token.value << std::endl;
            throw std::runtime_error("Error parsing JSON: Unknown token type");
        }
    }

    return node;
}

JsonLoader::Token JsonLoader::getToken() {
    if (fileStream.eof()) {
        throw std::logic_error("Ran out of tokens!");
    }

    char c;

    prevPos = fileStream.tellg();
    c = getWithoutWhiteSpace();

    Token token;

    if (c == '"') {
        token.type = Token::Type::STRING;
        token.value = "";

        fileStream.get(c);
        while (c != '"') {
            token.value += c;
            fileStream.get(c);
        }
    } else if (c == '-' || std::isdigit(c)) {
        token.type = Token::Type::NUMBER;
        token.value = "";
        token.value += c;

        std::streampos prevCharPos = fileStream.tellg();

        while (c == '-' || c == '+' || c == 'e' || c == '.' || std::isdigit(c)) { // check for scientific notation as well
            prevCharPos = fileStream.tellg();
            fileStream.get(c);

            if (fileStream.eof()) {
                break;
            } else if (c == '-' || c == '+' || c == 'e' || c == '.' || std::isdigit(c)) {
                token.value += c;
            } else {
                fileStream.seekg(prevCharPos);
            }
        }
    } else if (c == '{') {
        token.type = Token::Type::CURLY_OPEN;
        token.value = "{";
    } else if (c == '}') {
        token.type = Token::Type::CURLY_CLOSE;
        token.value = "}";
    } else if (c == '[') {
        token.type = Token::Type::ARRAY_OPEN;
        token.value = "[";
    } else if (c == ']') {
        token.type = Token::Type::ARRAY_CLOSE;
        token.value = "]";
    } else if (c == ':') {
        token.type = Token::Type::COLON;
        token.value = ":";
    } else if (c == ',') {
        token.type = Token::Type::COMMA;
        token.value = ",";
    }

    return token;
}

char JsonLoader::getWithoutWhiteSpace() {
    char c = ' ';

    while (std::isspace(c)) {
        fileStream.get(c);

        if (!fileStream.good()) {
            if (std::isspace(c)) {
                throw std::logic_error("Ran out of tokens!");
            } else {
                return c;
            }
        } 
    }

    return c;
}

void JsonLoader::rollbackToken() {
    if (fileStream.eof()) {
        fileStream.clear();
    }

    fileStream.seekg(prevPos);
}

void JsonLoader::close() {
    fileStream.close();
}