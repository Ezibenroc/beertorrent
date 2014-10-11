# BeerTorrent

Implementation of a simplified peer to peer protocol from scratch.

* Communication with sockets.
* Multithreading with pthread.

The tracker program and the protocol were given, we only describe in the present 
report the client program.


## General structure

Operations at the start of the program:

1. For each file, get the associated informations (hash, file size, pieces size, 
list of peers) from the tracker.
2. For each couple (peer, file), create a socket and realize a handshake with
the peer.

Then, several threads are created:

* **USER** Simple user interface. If the user enter `q`, then the program terminates.

* **WATCHER** Watching of all registered sockets, using the function `poll`. This
function blocks the thread till there is an available socket (i.e. a socket with
non-blocking reading). The `watcher` thread then put all readable sockets in the 
queue `request`. These sockets will then be poped by one of the thread `sender`
threads (defined after). The socket's messages will be handled, then the `sender` 
threads will put them in the queue `handled_request`. The `watcher` thread will
pop them to put them back in the watching set of the `poll` function.
We apply here a (modified) solution of the *bounded buffer problem*, to avoid 
deadlocks and starvation.

* **SENDER** There is a pool of several `sender` threads. We make a loop on the
following: pop a socket from the `request` queue (if there is none, the thread
is blocked by the semaphore), read the message, realize corresponding actions 
(defined after), put the socket back in the queue `handled_request` (protected
by a mutex).

* **LISTENER** Add new peers (using the function `listen`).

These threads are created in this order.

At the end of the program (i.e. when the user enter `q`), we terminates all loops
by putting a global boolean variable to true. The `listener` thread is blocked by
the `listen` function, we have to stop it by using the function `pthread_cancel`.


## Actions

We define some actions to perform when some specific messages are received. These
actions are performed by the `sender` threads.

* **Handshake** Send bitfield (if non null number of possessed pieces).

* **Have** Update corresponding bitfield.

* **Bitfield** Update of the bitfields. If it remains some pieces to ask, emit
some request(s) (not necessary to this peer).

* **Request** Send the asked piece.

* **Piece** Update the file. If it remains some pieces to ask, emit
some request(s) (not necessary to this peer).

Messages **Cancel** and **Keep-Alive** are ignored.

When a client begins to download some files, it emits several requests (because 
it has received bitfield(s)). A request is emitted for each received piece, thus
the number of ongoing requets remains constant. This number is in general greater
than 1 (if the files to download are big enough, it is equal to the constant
N_REQUETS). This allow a pipeline: when a client receive some piece, an other client
is already sending it another piece.


## Number of threads

The number of `sender` threads is specified at compile time (constant N_THREAD).
The threads are making a lot of input/output operations, the CPU would be often
idle if there was not enough threads. The number of ressources is limited (semaphores
and mutex for instance), thus a too large number of threads would be useless 
(all these threads would be blocked).

It would be interesting to perform some tests, to measure the downloading time for
several number of threads.


## Deadlocks avoidance

Several mutex are used, it is crucial to use deadlocks avoidance technics.

Firstly, we try for each thread not to lock two mutexes silmutaneously, to avoid
the condition *hold and wait* wich is necessary for a deadlock.

It is sometime necessary to lock several mutexes. In these cases, we define a total
order on the mutexes, to avoid the condition *circular wait* which is also necessary
for a deadlock.


## Peer choice strategy

We present here the strategy used for the choice of the peers.

* Select a file randomly.
* For this file, select the next piece deterministicaly (in increasing order).
* Select a peer randomly.
* If the random peer selection fails, then select deterministicaly the peer 
(take the first one which have the desired piece).
* If the peer selection fails again, then choose deterministicaly the file, 
repeating this strategy for the piece and the peer.

We thus choose randomly the file and the peer when there remains enough non-downloaded
pieces. This avoid a situation where all clients would send their requets to the 
same peer.
When there is not enough remaining pieces, we choose deterministicaly to ensure
the termination.


## Test

The program has been successfully tested for the exchange of two files between three peers.

**First test:** one peer have the two files, the two others do not have any.

**Second test:** two peers have the two files, the third one do not have any.

**Third test:** a peer have the first file, another peer have the other file, the last
peer do not have any.


## Conclusion

The program works correctly. It has poor performances.

Necessary code improvements: better structuration, avoid the usage of some global
variables, reduce code redudancy.

Necessary program improvements: implement timeouts on sockets to delete a peer when
it does not send any message, implement *Keep-Alive* messages, implement *Cancel*
messages. Implement other strategies (restart from partial download for instance).
Understand why the performances are low, and correct it.
