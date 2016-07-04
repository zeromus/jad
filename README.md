JAD
===

This project is pre-alpha WORK IN PROGRESS.

The JAD (Just-A-Disc) project is an implementation of the (new) JAD (and JAC) file formats dedicated for storing compressed optical discs in a random-access structure. Focus is on a healthy ecosystem and not just the format: tools and readily reusable libraries are provided.

JAD functions as a canonical representation of a disc.  It should be superior to cue+bin, ccd, etc. in this respect.  JAC functions as a conveniently compressed variant of a JAD.

It is designed to be simple and compact for embedded platforms; consequently the storage is not as efficient as it could be.  We have opted for a 'least common denominator' approach, with no bulky external dependencies which might be difficult to implement on every device.

It is similar to but less powerful than MAME's CHD system.

These JAC files are not necessarily supposed to be distributed directly, although that would be nice. Rather, they are designed to to be a representation of the disc suitable for hardware emulation.  Users would most likely convert other disc images to this format as an offline process.

The project is comprised of three parts: libjad, which solely implements the JAD/JAC formats; libjadvac which implements readers for other formats (cue,ccd) and conditionally encompasses much bulkier dependencies like libavcodec, etc. for decoding audio; and jadtool, which converts discs to JAD/JAC format. The intention is that libjad would be used on its own by most projects, libjadvac additionally by projects unwilling to require use of JAD/JAC files, and jadtool not used at all by other projects, but rather by end-users.