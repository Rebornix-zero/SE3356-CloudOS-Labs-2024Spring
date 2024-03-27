ovs-vsctl clear port s1-eth1 qos
ovs-vsctl clear port s1-eth2 qos
ovs-vsctl clear port s1-eth3 qos
ovs-vsctl clear port s1-eth4 qos
ovs-vsctl -- --all destroy qos -- --all destroy queue

