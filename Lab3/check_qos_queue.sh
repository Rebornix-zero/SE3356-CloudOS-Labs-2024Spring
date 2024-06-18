echo ">>>>list qos:"
ovs-vsctl list qos
echo "\n>>>>list queue:"
ovs-vsctl list queue
echo "\n>>>>show s1:"
ovs-ofctl show s1 -O openflow13
echo "\n>>>>dump-flows s1:"
ovs-ofctl dump-flows s1 -O openflow13