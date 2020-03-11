fixspec:getenv `FIXSPEC
fixconf:getenv `FIXCONF
fixbegin:getenv `FIXBEGIN

.fix:(`:./kdbfix.0.6.0 2:(`LoadLibrary;1))fixspec
\l ./tests/k4unit.q
\l ./tests/tags.q


stype:$[.z.x[0]~enlist "a";`acceptor;`initiator]
create:{.fix.create[stype;`$fixconf]}
create[]


store:()
.fix.onrecv:{[x]
    show x;
    .e.e:x;
    store::store,enlist x
    }

.fix.send_IOI:{[a;b]
    message:()!();
    message[.fix.tags.BeginString]: `$fixbegin;    		/-8
    message[.fix.tags.SenderCompID]: a;             		/-49
    message[.fix.tags.TargetCompID]: b;             		/-56
    message[.fix.tags.MsgType]: `6;                 		/-35
    message[.fix.tags.IOITransType]: `N;           		/-28
    message[.fix.tags.IOIid]:`$"SHD2015.04.04";    		/-23
    message[.fix.tags.IOIQty]: `L;                 		/-27
    message[.fix.tags.Symbol]: `TESTSYM;           		/-55
    message[.fix.tags.Side]: `1;                   		/-54
    message[.fix.tags.TransactTime]: `$"20150416-17:38:21";	/-60
    .fix.send[string message];
   }


if[stype~`initiator;KUltf[`$":tests/unittest.csv"]]
if[stype~`initiator;KUrt[]]

