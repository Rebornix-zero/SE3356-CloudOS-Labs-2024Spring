/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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
#include <deque>
#include "rdt_struct.h"
#include "rdt_sender.h"
#include "./rdt_utils.h"

void Sender_Timeout_GBN();
void Sender_FromLowerLayer_GBN(struct packet *pkt);
void Sender_FromUpperLayer_GBN(struct message *msg);

/* GBK滑动窗口允许的最大同时发包数 */
static uint slide_window_number = 16;
/**
 * 当前滑窗第一个pkt的id 也即首个未确认的pkt的id
 * 注意：pkt id是**1base**的
 */
static uint base;
/* 下一个待发送的pkt的id,则next_send_id-1即为当前已经发送出去的最大pkt id */
static uint next_send_id;
/* 下一个待生成的pkt的id,则next_send_id-1即为当前已经发送出去的最大pkt id*/
static uint next_create_id;

/**
 * 双端队列，用于存储未确认/未发出的包，顺序存储
 * 一旦确认某些包，其将按顺序从队列中移除
 */
static std::deque<struct packet *> pkt_list;
/**
 * 有效数据传输报文格式：
 * |<- 8 bytes          ->||<- 1 bytes      ->||<- 4 bytes ->||<- the rest  ->|
 * |<- hash checksum    ->||<- payload size ->||<- pkt id  ->||<- payload   ->|
 */
static uint hash_value_size = 8;
static uint length_size = 1;
static uint pkt_id_size = 4;
static uint max_payload_size = RDT_PKTSIZE - hash_value_size - length_size - pkt_id_size;
/* 默认超时时间 */
static double out_time = 0.3;

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    base = 1;
    next_create_id = 1;
    next_send_id = 1;
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    // /* 1-byte header indicating the size of the payload */
    // int header_size = 1;

    // /* maximum payload size */
    // int maxpayload_size = RDT_PKTSIZE - header_size;

    // /* split the message if it is too big */

    // /* reuse the same packet data structure */
    // packet pkt;

    // /* the cursor always points to the first unsent byte in the message */
    // int cursor = 0;

    // while (msg->size - cursor > maxpayload_size)
    // {
    //     /* fill in the packet */
    //     pkt.data[0] = maxpayload_size;
    //     memcpy(pkt.data + header_size, msg->data + cursor, maxpayload_size);

    //     /* send it out through the lower layer */
    //     Sender_ToLowerLayer(&pkt);

    //     /* move the cursor */
    //     cursor += maxpayload_size;
    // }

    // /* send out the last packet */
    // if (msg->size > cursor)
    // {
    //     /* fill in the packet */
    //     pkt.data[0] = msg->size - cursor;
    //     memcpy(pkt.data + header_size, msg->data + cursor, pkt.data[0]);

    //     /* send it out through the lower layer */
    //     Sender_ToLowerLayer(&pkt);
    // }
    Sender_FromUpperLayer_GBN(msg);
}

/* event handler, called when a packet is passed from the lower layer at the
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    Sender_FromLowerLayer_GBN(pkt);
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    Sender_Timeout_GBN();
}

/* GBN策略下，实现高吞吐rdt函数 */

/// @brief 将pkt_list的第一个元素删除，并释放其申请的堆空间
void pop_front_with_delete()
{
    delete pkt_list[0];
    pkt_list.pop_front();
}

/// @brief 将滑动窗口填满
void fullfill_windows()
{
    while ((next_send_id - base < slide_window_number) && (next_send_id <= next_create_id - 1))
    {
        /*如果之前没有发送并等待确认的包，则需要开启timer*/
        if (base == next_send_id)
        {
            Sender_StartTimer(out_time);
        }
        Sender_ToLowerLayer(pkt_list[next_send_id - base]);
        next_send_id += 1;
    }
}

void Sender_FromUpperLayer_GBN(struct message *msg)
{
    // 将message拆分为pkt,按序存入list
    // 可以凑满一个pkt的部分
    struct packet *pkt;
    uint cursor = 0;
    while (cursor + max_payload_size - 1 <= msg->size - 1)
    {
        pkt = new (struct packet)();
        rdt_utils::create_payload_pkt(pkt, (uint8_t)(max_payload_size), next_create_id, (msg->data) + cursor);
        pkt_list.push_back(pkt);
        cursor += max_payload_size;
        next_create_id += 1;
    }
    // 剩余的残缺部分
    if (cursor <= msg->size - 1)
    {
        pkt = new (struct packet)();
        rdt_utils::create_payload_pkt(pkt, (uint8_t)(msg->size - cursor), next_create_id, (msg->data) + cursor);
        pkt_list.push_back(pkt);
        next_create_id += 1;
    }
    // 将发送的包数量补至最大数N
    fullfill_windows();
}

void Sender_FromLowerLayer_GBN(struct packet *pkt)
{
    // hash校验
    if (rdt_utils::check_hash_from_pkt(pkt->data, rdt_utils::pkt_class::ACK))
    {
        uint ack_pktid = rdt_utils::get_id_from_pkt(pkt->data, rdt_utils::pkt_class::ACK);
        // 在hash校验成功的前提下，立刻检查pktid,如果其更新后将不再有包等待，立刻停止时钟
        if (ack_pktid + 1 == next_send_id)
        {
            Sender_StopTimer();
        }

        if (ack_pktid > base - 1)
        {
            // 有新确认的包
            // 将已经确认的包从pkt_list中删除
            for (uint i = base; i - 1 < ack_pktid; ++i)
            {
                pop_front_with_delete();
            }
            // 更新base和next_send_id
            base = ack_pktid + 1;
            if (next_send_id < base)
            {
                next_send_id = base;
            }
        }
    }
}

void Sender_Timeout_GBN()
{
    // 重启timer
    Sender_StartTimer(out_time);
    // 重发未确认的包
    uint pkt_num = next_send_id - base;
    for (uint i = 0; i < pkt_num; ++i)
    {
        Sender_ToLowerLayer(pkt_list[i]);
    }
}