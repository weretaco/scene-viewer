#ifndef _JSON_LOADER_H
#define _JSON_LOADER_H

#include <fstream>
#include <map>
#include <string>
#include <variant>
#include <vector>

class JsonLoader {
public:

    struct Token {
        enum class Type {
            STRING,
            NUMBER,
            ARRAY_OPEN,
            ARRAY_CLOSE,
            CURLY_OPEN,
            CURLY_CLOSE,
            COLON,
            COMMA
        };
        std::string value;
        Type type;
    };

    struct JsonNode {
        enum class Type {
            OBJECT,
            ARRAY,
            STRING,
            NUMBER
        };
        std::variant<
            std::map<std::string, JsonNode*>*,
            std::vector<JsonNode*>*,
            std::string,
            float
        > value;
        Type type;
    };

    JsonLoader(std::string filename);

    JsonNode* parseJson();
    JsonNode* parseObject();
    JsonNode* parseArray();
    JsonNode* parseString();
    JsonNode* parseNumber();
    JsonNode* parseNode();

    void close();

private:

    std::fstream fileStream;
    JsonNode* root;
    std::streampos prevPos;

    Token getToken();
    char getWithoutWhiteSpace();
    void rollbackToken();
};

#endif // _JSON_LOADER_H