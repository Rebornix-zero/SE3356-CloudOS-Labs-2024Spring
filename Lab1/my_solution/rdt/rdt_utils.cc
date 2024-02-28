#include "./rdt_utils.h"
#include <cstring>

namespace rdt_utils
{
    std::hash<std::string> rdt_hasher;

    uint64_t create_hash_value(const char *text, u_int length)
    {
        std::string hash_str(text, length);
        uint64_t hash_value = rdt_hasher(hash_str);
        return hash_value;
    }

    std::string create_ack_content(uint pkt_id)
    {
        std::string result(12, char(0));
        *(reinterpret_cast<uint *>(result.data())) = pkt_id;
        uint64_t hash_value = create_hash_value(result.data(), 4);
        *(reinterpret_cast<uint64_t *>(result.data() + 4)) = hash_value;
        return result;
    }

    bool check_hash_from_pkt(char *pkt_data, pkt_class pk_cl)
    {
        if (pk_cl == pkt_class::ACK)
        {
            uint64_t hash_in_pkt = *(reinterpret_cast<uint64_t *>(pkt_data + 4));
            return hash_in_pkt == create_hash_value(pkt_data, 4);
        }
        else
        {
            uint64_t hash_in_pkt = *(reinterpret_cast<uint64_t *>(pkt_data));
            uint8_t payload_size = *(reinterpret_cast<uint8_t *>(pkt_data + 8));
            return hash_in_pkt == create_hash_value(pkt_data + 8, 5 + payload_size);
        }
    }

    uint get_id_from_pkt(char *pkt, pkt_class cla)
    {
        uint result;
        if (cla == pkt_class::ACK)
        {
            result = *(reinterpret_cast<uint *>(pkt));
        }
        else
        {
            result = *(reinterpret_cast<uint *>(pkt + 9));
        }
        return result;
    }

    bool fill_msg(struct packet *src, struct message *dst)
    {
        /*装填size*/
        dst->size = *(reinterpret_cast<uint8_t *>((src->data) + 8));
        /*装填data*/
        dst->data = (char *)malloc(dst->size);
        memcpy(dst->data, src->data + 13, dst->size);
        return true;
    }

    bool create_payload_pkt(struct packet *result_pkt, uint8_t len, uint32_t pkt_id, char *payload)
    {
        memcpy((result_pkt->data) + 8, &len, 1);
        memcpy((result_pkt->data)+9,&pkt_id,4);
        memcpy((result_pkt->data)+13,payload,len);
        uint64_t hash_value=create_hash_value((result_pkt->data) + 8,len+5);
        memcpy(result_pkt->data,&hash_value,8);
        return true;
    }
}
