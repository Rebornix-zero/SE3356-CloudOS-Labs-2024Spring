export PATH=$PATH:/usr/local/share/openvswitch/scripts

#配置ovsdb的数据库
mkdir -p /usr/local/etc/openvswitch
ovsdb-tool create /usr/local/etc/openvswitch/conf.db \
vswitchd/vswitch.ovsschema
mkdir -p /usr/local/var/run/openvswitch
ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock \
    --remote=db:Open_vSwitch,Open_vSwitch,manager_options \
    --private-key=db:Open_vSwitch,SSL,private_key \
    --certificate=db:Open_vSwitch,SSL,certificate \
    --bootstrap-ca-cert=db:Open_vSwitch,SSL,ca_cert \
    --pidfile --detach --log-file

#使⽤ovs-vsctl初始化数据库
ovs-vsctl --no-wait init

#启动ovs-vswitchd
ovs-ctl --no-ovsdb-server start