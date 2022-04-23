Starting Servers (Acceptors) and Clients (Initiators)
----------------

The .fix.create function is common to both starting Acceptors and Initiators.

The Acceptor is the server side component of a FIX engine that provides some sort of service by binding to a port and accepting messages. To start an acceptor you need to call the .fix.create function with acceptor as the first argument. The second argument may be either an empty list or a specified configuration, e.g :sessions/sample.ini. The .fix.create function called as an acceptor will start a background thread that will receive and validate messages and finally forward them to the .fix.onrecv function if the message is well formed.

```
/ q fix.q
q) .fix.create[`acceptor;()]
Defaulting to sample.ini config
Creating Acceptor
```

The Initiator is the client side component of the FIX engine and is intended to be used to connect to acceptors to send messages. To start an initiator you need to call the .fix.create function. This will create a background thread that will open a socket to the target acceptor and allow you to send/receive messages.

```
/ q fix.q
q) .fix.create[`initiator;`:sessions/sample.ini]
Creating Initiator
```

Sending a FIX Message
--------------------

In order to send a FIX message from an initiator to an acceptor, you can use the .fix.send function. The send is executed synchronously and will either raise a signal upon error, otherwise you can assume that the message has been received successfully by the counterparty.

In order to determine who the message will be sent to, the library will read the contents the message dictionary and look for a session on the same process that matches. The BeginString, SenderCompID and TargetCompID fields must be present in every message for this reason.

```
/ Session 1 - Create an Acceptor
/ q fix.q
q) .fix.create[`acceptor;()]
Defaulting to sample.ini config
Creating Acceptor

/ Session 2 - Create an Initiator
/ q fix.q
q) .fix.create[`initiator;()]
Defaulting to sample.ini config
Creating Initiator

/ We can create a dictionary
/ containing tags and message values
q) message: (8 11 35 46 ...)!("FIX.4.4";175;enlist "D";enlist "8" ...)
/ Then send it as a message
q) .fix.send[message]
```
