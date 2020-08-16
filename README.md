# runit

This repository holds the version of runit that is used by Void Linux. It
incorporates patches that fix issues found by users as well as certain compiler
warnings.

The source history was obtained from <http://smarden.org/git/runit.git/>, but
the release tarballs have been pruned from this version.

The objective of this repository is not to revamp the runit code completely or
add functionality that detracts from its simplicity, but rather to provide a
canonical version of the source code and to avoid the inclusion of patches in
[void-packages](https://github.com/void-linux/void-packages). This also makes
reviewing patches much simpler. If you have an issue or patch that you feel fits
inside these objectives, please open an issue or pull request!
