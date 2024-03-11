#ifndef RDT_UTILS
#define RDT_UTILS

#include <functional>
#include <string>
#include <cstdint>
#include "./rdt_struct.h"

namespace rdt_utils
{
    /* public */
    enum pkt_class
    {
        ACK,
        PAYLOAD
    };
    /* 生成hash值 */
    uint64_t create_hash_value(const char *, u_int);
    /* 创建ack报文内容 */
    std::string create_ack_content(uint);
    /* 验证下层包的hash */
    bool check_hash_from_pkt(char *, pkt_class);
    /* 取出包中的pkt_id */
    uint get_id_from_pkt(char *, pkt_class);
    /**
     * 使用payload pkt内容装填message
     * 注意message的data字段应该由该函数内部进行malloc,但是回收要交给外层函数
     */
    bool fill_msg(struct packet *, struct message *);
    /* 传入payload size，pkt id和payload，构建一个pkt */
    bool create_payload_pkt(struct packet *, uint8_t, uint32_t, char *);
}

#endif