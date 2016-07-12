Issue a series of commands and arguments to be executed _in order_. Don't use -this or --this
* `v` - enable verbose debugging
* `jad <infile> <outfile>` - converts infile to outfile in jad format
* `jac <infile> <outfile>` - converts infile to outfile in jac format
* `<in|out|inout> <jad|vac|mirage|mednafen>` - demands specified API to be used for IO
* `test <infile>` - tests the infile, which must be a jad or jac, for hash validity. (TODO: which hash?)

note: the output API may seem pointless now (since this tool can only convert to jad/jac)
however, in the future, we may at least support writing cue+bin or ccd via libjadvac for comparison purposes
