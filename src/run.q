.fix.create_acceptor:`:./fixengine 2:(`run_fix_acceptor;1)
.fix.create_initiator:`:./fixengine 2:(`run_fix_initiator;1)

.fix.lookup_acceptor:`:./fixengine 2:(`lookup_acceptor;1)

.fix.send_new_order_single:`:./fixengine 2:(`send_new_order_single;1)
.fix.send_order_cancel:`:./fixengine 2:(`send_order_cancel;1)

.fix.create:{-1".fix.create"}
.fix.logout:{-1".fix.logout"}
.fix.login:{-1".fix.login"}
.fix.fromadmin:{-1".fix.fromadmin"}
.fix.toadmin:{-1".fix.toadmin"}
.fix.fromapp:{-1".fix.fromapp"}
.fix.toapp:{-1".fix.toapp"}
.fix.onmessage:{-1".fix.onmessage"}

.fix.globalconfig:`ReconnectInterval`HeartBtInt`PersistMessages`FileStorePath!(30;15;"Y";"store")
