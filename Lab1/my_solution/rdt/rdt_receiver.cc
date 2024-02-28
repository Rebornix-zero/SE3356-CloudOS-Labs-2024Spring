/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or
 *       reordering.  You will need to enhance it to deal with all these
 *       situations.  In this implementation, the packet format is laid out as
 *       the following:
 *
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_utils.h"

void Receiver_FromLowerLayer_GBN(struct packet *pkt);

/* 下一个等待的包的id */
static uint expected_seqnum;
/**
 * 根据expected_seqnum生成的ACK报文
 * 每当expected_seqnum更新，其也会随之更新一次
 * ack报文格式：
 *      |<- 4 bytes            ->|<- 8 bytes         ->|
 *      |<- max receive pkt id ->|<- hash checksum   ->|
 * max receive pkt id代表了按照顺序接受pkt后，当前**接受到的**最大pkt id
 * max receive pkt id初始为0，因为pkt id是1bash的，0代表暂未接受到包
 */
static std::string ack_content;
static uint ack_content_size = 12;

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());

    /* 初始化expected_seqnum,其初始值为1 */
    expected_seqnum = 1;
    ack_content = rdt_utils::create_ack_content(expected_seqnum - 1);
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    // /* 1-byte header indicating the size of the payload */
    // int header_size = 1;

    // /* construct a message and deliver to the upper layer */
    // struct message *msg = (struct message *)malloc(sizeof(struct message));
    // ASSERT(msg != NULL);

    // msg->size = pkt->data[0];

    // /* sanity check in case the packet is corrupted */
    // if (msg->size < 0)
    //     msg->size = 0;
    // if (msg->size > RDT_PKTSIZE - header_size)
    //     msg->size = RDT_PKTSIZE - header_size;

    // msg->data = (char *)malloc(msg->size);
    // ASSERT(msg->data != NULL);
    // memcpy(msg->data, pkt->data + header_size, msg->size);
    // Receiver_ToUpperLayer(msg);

    // /* don't forget to free the space */
    // if (msg->data != NULL)
    //     free(msg->data);
    // if (msg != NULL)
    //     free(msg);
    Receiver_FromLowerLayer_GBN(pkt);
}

void Receiver_FromLowerLayer_GBN(struct packet *pkt)
{
    struct packet ack_pkt;
    /*进行hash校验，并返回元数据*/
    bool is_notcurrupt = rdt_utils::check_hash_from_pkt(pkt->data, rdt_utils::pkt_class::PAYLOAD);
    bool is_expetced_id = (expected_seqnum == rdt_utils::get_id_from_pkt(pkt->data, rdt_utils::pkt_class::PAYLOAD));

    if (is_notcurrupt && is_expetced_id)
    {
        /* hash校验成功，且此包id与expect_id一致 */
        /*将包的数据封装为message格式转发给上层*/
        struct message upper_msg;
        rdt_utils::fill_msg(pkt, &upper_msg);
        Receiver_ToUpperLayer(&upper_msg);

        /*expect+1,并更新ack报文*/
        expected_seqnum += 1;
        ack_content = rdt_utils::create_ack_content(expected_seqnum - 1);
        /*发送ack报文*/
        memcpy(pkt->data, ack_content.data(), ack_content_size);
        Receiver_ToLowerLayer(&ack_pkt);

        /*释放空间*/
        if (upper_msg.data != NULL)
            free(upper_msg.data);
    }
    else
    {
        /* 其他情况，存在问题*/
        /* 重发一次上一个id的ack报文*/
        memcpy(pkt->data, ack_content.data(), ack_content_size);
        Receiver_ToLowerLayer(&ack_pkt);
    }
}