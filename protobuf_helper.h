#ifndef _PROTOBUF_HELPER_H_
#define _PROTOBUF_HELPER_H_

#include <stdint.h>
#include <string>
#include <memory>
#include <thread>
#include <google/protobuf/message.h>

using namespace google::protobuf;

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

namespace helper{
// 动态生成消息
inline MessagePtr  createMessage(const std::string& type_name){
    MessagePtr message = NULL;
	const Descriptor * descriptor
		= DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	if (descriptor){
		const Message * prototype
			= MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype){
			message =  MessagePtr(prototype->New());
		}
	} 

	return message;
}
}; // namespace

#endif
