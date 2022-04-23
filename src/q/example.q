\l fix.q

.fix.onrecv:{[x]
    show x;
    .e.e:x;
    if[x[35]~enlist "D"; .fix.send_execution_report[`$x[.fix.tags.TargetCompID];`$x[.fix.tags.SenderCompID]]];
  }

.fix.send_new_single_order: {[a;b]
    message:()!();
    message[.fix.tags.BeginString]: "FIX.4.2";
    message[.fix.tags.SenderCompID]: string a;
    message[.fix.tags.TargetCompID]: string b;
    message[.fix.tags.MsgType]: enlist "D";
    message[.fix.tags.ClOrdID]: "SHD2015.04.04";
    message[.fix.tags.Symbol]: "TESTSYM";
    message[.fix.tags.Side]: enlist "2";
    message[.fix.tags.HandlInst]: enlist "2";
    message[.fix.tags.TransactTime]: 2016.03.04D14:21:36.567000000;
    message[.fix.tags.OrdType]: enlist "1";
    .fix.send[message];
    }

.fix.send_execution_report: {[a;b]
    message:()!();
    message[.fix.tags.MsgType]: enlist "8";
    message[.fix.tags.BeginString]: "FIX.4.2";
    message[.fix.tags.SenderCompID]: string a;
    message[.fix.tags.TargetCompID]: string b;
    message[.fix.tags.OrderID]: "ORDER2015.04.04";
    message[.fix.tags.ExecID]: "ORDER2015.04.04";
    message[.fix.tags.ExecType]: enlist "0";
    message[.fix.tags.OrdStatus]: enlist "0";
    message[.fix.tags.Symbol]: "TESTSYM";
    message[.fix.tags.Side]: enlist "1";
    message[.fix.tags.LeavesQty]: 123.4;
    message[.fix.tags.CumQty]: 876.0;
    message[.fix.tags.AvgPx]: 1.38;
    message[.fix.tags.ExecTransType]: enlist "0";
    .fix.send[message];
   }

.fix.send_IOI:{[a;b]
    message:()!();
    message[.fix.tags.BeginString]: `$"FIX.4.2";    /-8
    message[.fix.tags.SenderCompID]: a;             /-49
    message[.fix.tags.TargetCompID]: b;             /-56
    message[.fix.tags.MsgType]: `6;                 /-35
    message[.fix.tags.IOITransType]: `N;           /-28
    message[.fix.tags.IOIid]:`$"SHD2015.04.04";    /-23
    message[.fix.tags.IOIQty]: `L;                 /-27
    message[.fix.tags.Symbol]: `TESTSYM;           /-55
    message[.fix.tags.Side]: `1;                   /-54
    message[.fix.tags.TransactTime]: `$"20150416-17:38:21";  /-60
    .fix.send[string message];
   }

.fix.send_message: {[a;b]
   message:()!();
   message[.fix.tags.BeginString]: "FIX.4.2";    / 8
   message[.fix.tags.SenderCompID]: string a;           / 49
   message[.fix.tags.TargetCompID]: string b;           / 56
   message[.fix.tags.MsgType]: "6";
   message[.fix.tags.Side]: "2";           / 54
   message[.fix.tags.Symbol]: "TESTSYM";
   message[.fix.tags.IOITransType]: "N";
   message[.fix.tags.IOIShares]: "100";
   .fix.send[message];
   }
