/*
    Smartt C++ Client
    Copyright 2013 S10I (s10i.com.br)

    Author: Felipe Menezes Machado (felipe@s10i.com.br)
*/

#include "smartt_simple_protocol.h"

#include <set>
#include <iomanip>
#include <sstream>

/*************************************************************************************************
  Helper functions
*************************************************************************************************/
// Escapes a string according to the protocol - avoid using the reserved ';' and '$' characters
string escape(const string &value) {
    static set<char> encoded_characters;
    if (encoded_characters.size() == 0)
    {
        encoded_characters.insert(';');
        encoded_characters.insert('$');
        encoded_characters.insert('\\');
        for (int i = 0; i < 32; ++i)
        {
            encoded_characters.insert((char)i);
        }
    }

    ostringstream encoded;
    for (unsigned int i = 0; i < value.size(); ++i)
    {
        char c = value[i];

        if( encoded_characters.find(c) == encoded_characters.end() )
        {
            encoded << c;
        }
        else
        {
            encoded << "\\x" << hex << setfill('0') << setw(2) << (int)c;
        }
    }

    return encoded.str();
}

// Unescapes a string according to the protocol
string unescape(const string &value) {
    ostringstream decoded;
    int hex_escape_count = 0;

    for (unsigned int i = 0; i < value.size(); ++i)
    {
        if (hex_escape_count == 0)
        {
            char c = value[i];
            if (c == '\\' && (i+3 < value.size()) && value[i+1] == 'x')
            {
                hex_escape_count = 3;
                char hex_string[2] = {value[i+2], value[i+3]};
                decoded << (char)strtol(hex_string, NULL, 16);
            }
            else
            {
                decoded << c;
            }
        }
        else
        {
            hex_escape_count -= 1;
        }
    }

    return decoded.str();
}

// Splits a string according to a separator, unescaping the tokens
vector<string> split(const string &s, const string &separator) {
    vector<string> tokens;

    size_t search_start_position = 0;
    size_t separator_position = string::npos;

    // Search for the separator in the string and store its position
    while( (separator_position = s.find(separator, search_start_position)) != string::npos ) {
        // Unescape the string between the separators
        string unescaped_string = unescape( s.substr(search_start_position, separator_position-search_start_position) );

        // Add the unescaped token to the list
        tokens.push_back( unescaped_string );

        // Walks in the string to the position after the current separator
        search_start_position = (separator_position+separator.size());
    }

    // Add the last token from the last separator to the send of the string
    string unescaped_string = unescape( s.substr(search_start_position) );
    if (unescaped_string != "")
    {
        tokens.push_back( unescaped_string );
    }

    return tokens;
}

// Joins the list of strings using the given separator
string join(const vector<string> &tokens, const string &separator) {
    string result = "";

    // Start with the first token if it exists
    if (tokens.size() > 0)
    {
        result = tokens[0];
    }

    // For each other token, append the separator and the escaped token
    for (unsigned int i = 1; i < tokens.size(); ++i)
    {
        result += (separator + escape(tokens[i]));
    }
    return result;
}


/*************************************************************************************************
  SmarttSimpleProtocol implementation
*************************************************************************************************/
SmarttSimpleProtocol::SmarttSimpleProtocol(SimpleStream &simpleStream) :
        stream(simpleStream)
{
    stream_receive_buffer_ = "";
}

void SmarttSimpleProtocol::sendMessage(const vector<string> &messageParts) {
    // Joins the strings using the helper function and appends the dollar sign at the end
    string message = join(messageParts, ";") + "$";

    stream.write(message);
}

vector<string> SmarttSimpleProtocol::receiveMessage() {
    size_t terminator_index = string::npos;

    // Buffer up data until the terminator
    while( (terminator_index = stream_receive_buffer_.find("$")) == string::npos ) {
        string data = stream.read();

        stream_receive_buffer_.append(data);
    }

    // Retrieve the complete message from the buffer
    string message = stream_receive_buffer_.substr(0, terminator_index);

    // Trucates the buffer by one message (removes the current message from the start of the
    // buffer)
    stream_receive_buffer_ = stream_receive_buffer_.substr( terminator_index+1 );

    // Split the message unescaping the tokens and return
    return split(message, ";");
}
