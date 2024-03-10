# Answer to Lab2 Part1

## Q1: What's the purpose of using hugepage?
Using hugepage can effectively reduce the TLB miss rate, and thus can reduce the costs and improve efficiency about memory access.

## Q2: Take examples/helloworld as an example, describe the execution flow of DPDK programs?
The code of dpdk/examples/helloworld/main.c is listed below:  
```C
static int
lcore_hello(__rte_unused void *arg)
{
	unsigned lcore_id;
	lcore_id = rte_lcore_id();
	printf("hello from core %u\n", lcore_id);
	return 0;
}

int
main(int argc, char **argv)
{
	int ret;
	unsigned lcore_id;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	/* call lcore_hello() on every worker lcore */
	RTE_LCORE_FOREACH_WORKER(lcore_id) {
		rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
	}

	/* call it on main lcore too */
	lcore_hello(NULL);

	rte_eal_mp_wait_lcore();

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}
```   
Function *lcore_hello()* will run in created threads. It can get the arguments that the caller send to it, and get its logical core id using *rte_local_id()*.   
Function *main()* is the entrance of the program. At first, it calls *rte_eal_init()* trying to init the runtime environment abstraction layer with argc and argv. If succeed, it uses *RTE_LCORE_FOREACH_SLAVE* to iterate all EAL, gets the slave lcore that can be used, and then calls *rte_eal_remote_launch()* to create the thread on them. Threads will execute *lcore_hello()*.  
Next, the main lcore call *lcore_hello()* too. And *rte_eal_mp_wait_lcore()* will block the program until all threads finish their works. Finally, *rte_eal_cleanup()* cleans up the EAL and then program ends.  

## Q3: Read the codes of examples/skeleton, describe DPDK APIs related to sending and receiving packets.

The DPDK APIs related to sending and receiving packets are *rte_eth_rx_burst(uint8_t port_id, uint16_t queue_id, struct rte_mbuf **rx_pkts, const uint16_t nb_pkts)* and *rte_eth_tx_burst(uint8_t port_id, uint16_t queue_id, struct rte_mbuf **tx_pkts, uint16_t nb_pkts)*.  

*rte_eth_rx_burst(uint8_t port_id, uint16_t queue_id, struct rte_mbuf **rx_pkts, const uint16_t nb_pkts)* is used to receive burst of packets:  
- port_id: the ethernet port ID.
- queue_id: the queue ID.
- rx_pkts: the array of rte_mbuf pointers that stores the received packets
- nb_pkts: the number of packets that need to be received.
- return: the number of packets successfully received.  

*rte_eth_tx_burst(uint8_t port_id, uint16_t queue_id, struct rte_mbuf **tx_pkts, uint16_t nb_pkts)* is used to receive burst of packets:  
- port_id: the ethernet port ID.
- queue_id: the queue ID.
- rx_pkts: the array of rte_mbuf pointers that stores the  packets to send.
- nb_pkts: the number of packets to send.
- return: the number of packets that successfully send.  

## Q4: Describe the data structure of 'rte_mbuf'  

rte_mbuf is the data structure used to describe the network data packets in the dpdk environment. The payload need to be sent ("payload" in this context points to the whole message in network, including the hierarchical protocol header and their body) is in the struct. Anyway, it also stores many metadata used for dpdk environment:   
- buf_addr: virtual address of mbuf
- buf_iova: physical address of mbuf
- data_off: the offest of mbuf data room to the received message.
- refcnt: the reference number of the mbuf
- nb_segs next: the data to describe the multiple segments of the packet. nb_segs is the number of segments. next is the pointer to next segment rte_mbuf.
- port: the io port number
- of_flags: the flag of offload
- union type: the type of packet
- union hash: the hash information od packet
- pool: the pool that allocate the mbuf
- cacheline0 cacheline1: describte two cache line to store different data.
- union tx_offload: to support TX offloads
