ovs-vsctl set port s1-eth1 qos=@qos1 -- --id=@qos1 create qos type=linux-htb queues=2=@q2,3=@q3,4=@q4 -- --id=@q2 create queue other-config:min-rate=5500000 other-config:max-rate=5600000 -- --id=@q3 create queue other-config:min-rate=3300000 other-config:max-rate=3400000 -- --id=@q4 create queue other-config:min-rate=0 other-config:max-rate=1200000

ovs-ofctl add-flow s1 cookie=2,in_port=2,action=set_queue:2,output:1  -O openflow13
ovs-ofctl add-flow s1 cookie=3,in_port=3,action=set_queue:3,output:1  -O openflow13
ovs-ofctl add-flow s1 cookie=4,in_port=4,action=set_queue:4,output:1  -O openflow13