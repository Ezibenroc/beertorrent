#Simplified peer to peer protocol.

Tom Cornebize, Agathe Herrou

Compilation: `make`

Create a file foo.beertorrent with a file foo.ext: `./torrent_maker foo.ext`

Launch the tracker: `./tracker`

Launch a client `./client [files]`

For technical details, read the associated report.

**Example:**

Consider two files foo.jpg bar.txt in the repository `beertorrent`.

We assume to be at root directory at each begining of these steps. User should
have several terminals to run these steps.

**1** Initialisation. Create the torrent file, copy it in directory WITHOUT (first terminal).
```
cd beertorrent
./../torrent_maker foo.jpg
./../torrent_maker bar.txt
mkdir ../WITHOUT
cp foo.beertorrent bar.beertorrent ../WITHOUT
```

**2** Launch the tracker (first terminal).
```
./tracker
```

**3** Laucn a first client which have the file (second terminal).
```
cd beertorrent
./../client foo.beertorrent bar.beertorrent
```

**4** Launch a second client which does not have the files (third terminal).
```
cd WITHOUT
./../client foo.beertorrent bar.beertorrent
```
