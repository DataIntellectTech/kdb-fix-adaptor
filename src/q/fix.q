.fix:(`:lib/fixengine 2:(`load_library;1))`

\l fixenums.q

.fix.send_new_single_order: {
    message:()!();
    
    message[.fix.tags.BeginString]: `$"FIX.4.2";
    message[.fix.tags.SenderCompID]: `BANZAI;
    message[.fix.tags.TargetCompID]: `FIXIMULATOR;
    message[.fix.tags.MsgType]: `D;
    message[.fix.tags.ClOrdID]: `$"SHD2015.04.04";
    message[.fix.tags.Symbol]: `TESTSYM;
    message[.fix.tags.Side]: `1;
    message[.fix.tags.HandlInst]: `2;
    message[.fix.tags.TransactTime]: `$"20150416-17:38:21";
    message[.fix.tags.OrdType]: `1; 

    .fix.send[message]; }

/ Create a simple acceptor to recieved and validate FIX messages.
/ .fix.listen[`SenderCompID`TargetCompID!`BuySideID`SellSideID];

/ Create a simple initiator that will connect to the acceptor.
/ .fix.connect[`SenderCompID`TargetCompID!`SellSideID`BuySideID];

/ Create a simple handler that just prints the recieved dictionary to the screen.
.fix.onrecv:{[d]
    show "------ START RECIEVED MESSAGE ------";
    show d;
    show "------  END RECIEVED MESSAGE  ------"; }

/ Send a valid example new order message from initiator to acceptor
/ .fix.send_new_single_order[];
