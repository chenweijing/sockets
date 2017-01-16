#include <stdio.h>
#include <arpa/inet.h>
#include <glog/logging.h>

#include "machine_study.pb.h"
#include "coder.h"

int main(int argc, char * argv[])
{
    mstudy::ImageData image;
    image.set_st_id("201701161439");
    image.set_bytes("Hello, This is image data.");
    image.set_resize_width(224);
    image.set_resize_height(224);
    std::string bytes;
    image.SerializeToString(&bytes);

    net::Coder coder;
    coder.setMsgName("mstudy.ImageData");
    coder.setBody(bytes);
    coder.encoding();

    string data = coder.geData();
    
    net::Coder decoder;
    uint32_t len = 0;
    memcpy((void*)&len, &*data.begin(), sizeof(uint32_t));
    
    LOG(INFO) << "data len : " << len << " " << ntohl(len);
    
    len = ntohl(len);

    decoder.decoding((char*)(&*data.begin() + sizeof(uint32_t)), len);

    LOG(INFO) << "encoding : msg name " << decoder.getMsgName();

    mstudy::ImageData image2;
    image2.ParseFromString(decoder.getBody());

    LOG(INFO) << "IAMGE: st_id: " << image2.st_id() << " data: " << image2.bytes() << " w " << image2.resize_width() << " h " << image2.resize_height();  

    return 0;
}
