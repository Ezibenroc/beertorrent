# BeerTorrent

Implementation of a simplified peer to peer protocol from scratch.

* Communication with sockets.
* Multithreading with pthread.

The tracker program was given, I only describe in the present report the client
program.

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
