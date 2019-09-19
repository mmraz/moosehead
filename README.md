# moosehead
MooseHead SLED circa 2005

master branch requires gcc-3.3 and a 32-bit OS, very obsolete

mhsled.com branch is what was running at that site late November 2015

ubuntu-15.04 branch is most current.  Works with gcc-5.2.1 on a 64bit OS and contains
a Dockerfile for a Docker image build.

It does have two requirements for libraries to be installed on the machine, libgc and libatomic_ops.
The following URLs are for packages known to work with the code:
http://hboehm.info/gc/gc_source/gc-7.4.2.tar.gz
http://hboehm.info/gc/gc_source/libatomic_ops-7.4.0.tar.gz
