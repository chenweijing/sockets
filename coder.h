#ifndef _CODE_H_
#define _CODE_H_

#include <stdint.h>
#include <memory.h>
#include <string>

using namespace std;

// data encode and decode
// format
// | len                 | -- sizeof (int32_t)
// | name len            | -- sizeof (int32_t)
// | msg name            | -- name len (name len)
// | body(protobuf data) | -- len - sizeof(int32_t) - sizeof(int32_t) - (name len) - sizeof(int32_t)
// | check sum           | -- sizeof (int32_t)
namespace net{
class Coder
{
public:
    Coder();
    ~Coder();
    // set Msg name
    void setMsgName(const string& name);
    // set msg data (protobuf data)
    void setBody(const string& body);
    // encoding the data to buff
    void encoding();

    // get Msg name
    string getMsgName();
    // get msg data (protobuf data)
    string getBody();
    // decoding the data from buff
    void decoding(const char * buff, int size);
private:
    int buf_size_;
    int buf_index_;
    char * buf_;
    int32_t len_;
    int32_t name_len_;
    string msg_name_;
    string body_;
    int32_t check_sum_;

    string data_;
}; // !class Coder
} // !namespace net
#endif
