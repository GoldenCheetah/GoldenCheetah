/*
 *
 *  Compact JSON format parsing lib (native cross-platform c++)
 *
 *  Copyright (C) 2013 Victor Laskin (victor.laskin@gmail.com)
 *  Details: http://vitiy.info/?p=102
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 *  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <string>
#include <vector>
#include <stdlib.h>

#ifndef MVJSON_H_
#define MVJSON_H_

using namespace std;

/// JSON node types
enum MVJSON_TYPE
{
    MVJSON_TYPE_NULL,
    MVJSON_TYPE_OBJECT,
    MVJSON_TYPE_ARRAY,
    MVJSON_TYPE_STRING,
    MVJSON_TYPE_INT,
    MVJSON_TYPE_DOUBLE,
    MVJSON_TYPE_BOOL
};

class MVJSONUtils {
protected:

    // string parsing functions
    inline static int stringToInt(const string & s);                                            ///< convert string to int
    inline static double stringToDouble(const string & s);                                      ///< convert string to float
    inline static bool symbolToBeTrimmed(const char& c);                                        ///< check if symbol is space, tab or new line break
    inline static string trim(const string & text);                                             ///< trim spaces, tabs and line break
    inline static void replace(string & target, const string & oldStr, const string & newStr);  ///< replace all occurrences of substring
    inline static void splitInHalf(const string & s, const string & separator, string & begin, string & end);   ///< second half (output)
    inline static void splitList(const string & s,  vector<string> & parts);
};


class MVJSONNode;

/// JSON Value
class MVJSONValue : public MVJSONUtils {
public:
    MVJSONValue(string name, MVJSON_TYPE valueType);
    MVJSONValue(string name, bool value);
    MVJSONValue(string name, string value);
    MVJSONValue(string name, int value);
    MVJSONValue(string name, double value);
    MVJSONValue(string name, MVJSONNode* value);
    virtual ~MVJSONValue();

    string name;                            ///< value name [optional]
    MVJSON_TYPE valueType;                  ///< type of node

    string stringValue;                     ///< value if data has string type
    bool boolValue;                         ///< value if data has bool type
    int intValue;                           ///< value if data has int type
    double doubleValue;                     ///< value if data has double type
    MVJSONNode* objValue;                   ///< value if data has object type

    vector<MVJSONValue*> arrayValue;        ///< array of values

    double getFieldDouble(string name);     ///< get value of double field of VALUE OBJECT (objValue)
    int getFieldInt(string name);           ///< get value of int field of VALUE OBJECT (objValue)
    string getFieldString(string name);     ///< get value of string field of VALUE OBJECT (objValue)
    bool getFieldBool(string name);         ///< get value of bool field of VALUE OBJECT (objValue)

    inline MVJSONValue* at(unsigned int i){ return arrayValue.at(i); }
    inline int size() { if (valueType == MVJSON_TYPE_ARRAY) return arrayValue.size(); else return 1; }
private:
    void init(MVJSON_TYPE valueType);
};

/// JSON Node (Object)
class MVJSONNode {
public:

    virtual ~MVJSONNode();

    vector<MVJSONValue*> values;            ///< values (props)

    bool hasField(string name);             ///< check that object has field
    MVJSONValue* getField(string name);     ///< get field by name

    double getFieldDouble(string name);     ///< get value of double field
    int getFieldInt(string name);           ///< get value of int field
    string getFieldString(string name);     ///< get value of string field
    bool getFieldBool(string name);         ///< get value of bool field
};

/// Compact JSON parser (based on specification: http://www.json.org/)
class MVJSONReader : public MVJSONUtils {
public:
    MVJSONReader(const string & source);    ///< constructor from json source
    virtual ~MVJSONReader();

    MVJSONNode* root;                       ///< root object (if its null - parsing was failed)

private:
    MVJSONValue* parseValue(string text, bool hasNoName);                   ///< parse value
    MVJSONNode* parse(string text);                                         ///< parse node


};



// ------------------- inlined string processing functions ------------->


inline int MVJSONUtils::stringToInt(const string & s)
{
    return atoi(s.c_str());
}


inline double MVJSONUtils::stringToDouble(const string & s)
{
    return atof(s.c_str());
}


inline bool MVJSONUtils::symbolToBeTrimmed(const char& c   ///< the char to test
)
{
    if (c == ' ') return true;
    if (c == '\n') return true;
    if (c == '\r') return true;
    if (c == '\t') return true;
    return false;
}


inline string  MVJSONUtils::trim(const string & text  ///< the text to trim
)
{
    if (text.length() == 0) return "";
    size_t start = 0;
    while ((start < text.length()) && (symbolToBeTrimmed(text[start]))) start++;
    size_t end = text.length() - 1;
    while ((end > 0) && (symbolToBeTrimmed(text[end]))) end--;
    if (start > end) return "";

    return text.substr(start, end - start + 1);
}

inline void MVJSONUtils::splitInHalf(const string & s,                      ///< source string to split up
                                      const string & separator,             ///< separator string)
                                      string & begin,                       ///< first part (output)
                                      string & end                          ///< second half (output)
)
{
    size_t pos = s.find(separator);
    if (pos == string::npos) { begin = s; end = ""; return; }

    begin = s.substr(0, pos);
    end = s.substr(pos + separator.length(), s.length() - pos - separator.length());
}

/// split string by "," - ignore content inside of "{", "}", "[", "]" and quotations "...."
/// also take \" into account
/// (Code should be cleared of comments beforehand)
inline void MVJSONUtils::splitList(const string & s,                ///< string to be splitted
                                    vector<string> & parts          ///< result parts
)
{

    bool isNotInQuotes = true;
    int b1 = 0;
    int b2 = 0;
    int lastPos = 0;

    const char* start = s.c_str();
    const char* ps = start;

    while (*ps) // *ps != 0
    {
        if ((*ps == ',') && (isNotInQuotes) && (b1 == 0) && (b2 == 0))
        {
            parts.push_back(s.substr(lastPos, ps - start - lastPos));
            lastPos = ps - start + 1;
        }

        if (isNotInQuotes)
        {
            if (*ps == '{') b1++;
            if (*ps == '}') b1--;
            if (*ps == '[') b2++;
            if (*ps == ']') b2--;
        }

        if (*ps == '"')
        {
            isNotInQuotes = !isNotInQuotes;
            if (ps != start)
                if (*(ps-1) == '\\')
                    isNotInQuotes = !isNotInQuotes;
        }

        ps++;
    }

    parts.push_back(s.substr(lastPos, s.length() - lastPos));
}

inline void MVJSONUtils::replace(string & target,           ///< text to be modified
                                  const string & oldStr,        ///< old string
                                  const string & newStr     ///< new string
)
{
    size_t pos = 0;
    const size_t oldLen = oldStr.length();
    const size_t newLen = newStr.length();

    for (;;)
    {
        pos = target.find(oldStr, pos);
        if (pos == string::npos) break;
        target.replace(pos, oldLen, newStr);
        pos += newLen;
    }
}
#endif /* MVJSON_H_ */
