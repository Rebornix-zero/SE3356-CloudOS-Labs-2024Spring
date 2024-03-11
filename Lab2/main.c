#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

// you can modify them
uint32_t src_port = 8080;
uint32_t dst_port = 8080;
char *dst_mac = "00-50-56-C0-00-01";
char *dst_ipv4 = "192.168.115.1";
char *src_ipv4 = "192.168.115.128";
char *message = "Hello Dpdk!";

static struct rte_mbuf *alloc_udp_pkt(struct rte_mempool *pool, uint8_t *data, uint16_t length, uint16_t port)
{
    int retval;
    // allocate a mbuf from mbuf pool
    struct rte_mbuf *mbuf = rte_pktmbuf_alloc(pool);
    if (!mbuf)
    {
        rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc error\n");
    }
    // the length of whole mbuf：the length of udp packet + the length of ip header + the length of ethernet header
    mbuf->pkt_len = length + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr);
    // the length of mbuf data
    mbuf->data_len = length + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ether_hdr);

    uint8_t *msg = rte_pktmbuf_mtod(mbuf, uint8_t *);

    // ether_hdr
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)msg;
    // naive ///////////////////////////////////////////////////////////////
    struct rte_ether_addr src_addr;
    retval = rte_eth_macaddr_get(port, &src_addr);
    if (retval != 0)
        return retval;
    printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
           " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
           port,
           src_addr.addr_bytes[0], src_addr.addr_bytes[1],
           src_addr.addr_bytes[2], src_addr.addr_bytes[3],
           src_addr.addr_bytes[4], src_addr.addr_bytes[5]);
    rte_memcpy(eth->s_addr.addr_bytes, (src_addr.addr_bytes), RTE_ETHER_ADDR_LEN);

    struct rte_ether_addr dst_addr;
    // transfer the destnation mac to 6-byte data
    sscanf(dst_mac, "%02hhx-%02hhx-%02hhx-%02hhx-%02hhx-%02hhx",
           &dst_addr.addr_bytes[0], &dst_addr.addr_bytes[1], &dst_addr.addr_bytes[2],
           &dst_addr.addr_bytes[3], &dst_addr.addr_bytes[4], &dst_addr.addr_bytes[5]);
    printf("dst mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
           dst_addr.addr_bytes[0], dst_addr.addr_bytes[1], dst_addr.addr_bytes[2],
           dst_addr.addr_bytes[3], dst_addr.addr_bytes[4], dst_addr.addr_bytes[5]);
    rte_memcpy(eth->d_addr.addr_bytes, (dst_addr.addr_bytes), RTE_ETHER_ADDR_LEN);
    ///////////////////////////////////////////////////////////////

    eth->ether_type = htons(RTE_ETHER_TYPE_IPV4);

    // iphdr
    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(msg + sizeof(struct rte_ether_hdr));
    ip->version_ihl = 0x45;
    ip->type_of_service = 0;
    ip->total_length = htons(length + sizeof(struct rte_ipv4_hdr));
    ip->packet_id = 0;
    ip->fragment_offset = 0;
    ip->time_to_live = 64;
    ip->next_proto_id = IPPROTO_UDP;
    // naive///////////////////////////////////////////////////////////////
    struct in_addr ip_addr;
    inet_aton(src_ipv4, &ip_addr);
    ip->src_addr = ip_addr.s_addr;
    inet_aton(dst_ipv4, &ip_addr);
    ip->dst_addr = ip_addr.s_addr;
    ip->hdr_checksum = 0;
    ip->hdr_checksum = rte_ipv4_cksum(ip);
    ///////////////////////////////////////////////////////////////

    // udphdr
    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)(msg + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
    // naive ///////////////////////////////////////////////////////////////
    udp->src_port = rte_cpu_to_be_16(src_port);
    udp->dst_port = rte_cpu_to_be_16(dst_port);
    ///////////////////////////////////////////////////////////////
    udp->dgram_len = htons(length);
    rte_memcpy((uint8_t *)(udp + 1), data, length - sizeof(struct rte_udp_hdr));
    udp->dgram_cksum = 0;
    udp->dgram_cksum = rte_ipv4_udptcp_cksum(ip, udp);

    return mbuf;
}

unsigned int MBUF_NUMBER = 8;
static const struct rte_eth_conf port_conf_default = {
    .rxmode = {
        .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
    },
};

int main(int argc, char *argv[])
{
    uint16_t portid;
    int retval;

    // 1、eal init
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    argc -= ret;
    argv += ret;

    // 2、create mbuf pool
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL)
    {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    // 3、config the network adapter
    // the number of quenes to receive/send packet
    uint16_t nb_rx_queues = 1;
    uint16_t nb_tx_queues = 1;

    // default configuration for network adapter
    struct rte_eth_conf port_conf = port_conf_default;

    for (portid = rte_eth_find_next_owned_by(0, 0); (unsigned int)portid < (unsigned int)32; portid = rte_eth_find_next_owned_by(portid + 1, 0))
    {
        printf("Start Init port %" PRIu16 "\n", portid);
        // configure network adapter
        retval = rte_eth_dev_configure(portid, nb_rx_queues, nb_tx_queues, &port_conf_default);
        if (retval != 0)
        {
            rte_exit(EXIT_FAILURE, "rte_eth_dev_configure error\n");
        }

        // create receive quene
        retval = rte_eth_rx_queue_setup(portid, 0, 128, rte_eth_dev_socket_id(portid), NULL, mbuf_pool);
        if (retval < 0)
        {
            rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup error\n");
        }

        // create send quene
        retval = rte_eth_tx_queue_setup(portid, 0, 1024, rte_eth_dev_socket_id(portid), NULL);
        if (retval < 0)
        {
            rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup error\n");
        }

        // start the network adapter
        retval = rte_eth_dev_start(portid);
        if (retval < 0)
        {
            rte_exit(EXIT_FAILURE, "rte_eth_dev_start error\n");
        }
    }

    // run main code to send udp message on the main core only.
    for (portid = rte_eth_find_next_owned_by(0, 0); (unsigned int)portid < (unsigned int)32; portid = rte_eth_find_next_owned_by(portid + 1, 0))
    {
        struct rte_mbuf *mbuf = alloc_udp_pkt(mbuf_pool, (uint8_t *)(message), sizeof(struct rte_udp_hdr) + strlen(message), portid);
        rte_eth_tx_burst(portid, 0, &mbuf, 1);
    }

    // clean up the EAL
    rte_eal_cleanup();
}
