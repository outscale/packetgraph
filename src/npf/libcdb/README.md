# NetBSD's constant database (cdb) library #

The NetBSD's constant database (cdb) library provides a space efficient
key-value database based on perfect hashing, thus guaranteeing the O(1)
lookup time.  The database preserves the key order.

This package provides a shared library.  Just `make rpm` to build an RPM.

## Algorithm ##

CHM-3: "An optimal algorithm for generating minimal perfect hash functions",
by Czech, Havas and Majewski in Information Processing Letters, 43(5):256-264, October 1992.

References:
- http://cmph.sourceforge.net/papers/chm92.pdf
- http://www.netbsd.org/gallery/presentations/joerg/asiabsdcon2013/hashing.pdf

## Examples ##

The following are the C code fragments to demonstrate the use of libcdb.

To create a cdb:

```c
#include <cdbw.h>
...

const char *key = "some-key";
const char *val = "some-key:some-val";
struct cdbw *cdb;

if ((cdb = cdbw_open()) == NULL)
	err(EXIT_FAILURE, "cdbw_open");

if (cdbw_put(cdb, key, strlen(key), val, strlen(val)) == -1)
	err(EXIT_FAILURE, "cdbw_put");

if (cdbw_output(cdb, fd, "my-cdb", NULL) == -1)
	err(EXIT_FAILURE, "cdbw_output");

cdbw_close(cdb);
```

To read a cdb:

```c
#include <cdbr.h>
...

const char key[] = "some-key";
const size_t keylen = sizeof(key) - 1;
struct cdbr *cdb;
const void *data;
size_t len;

if ((cdb = cdbr_open(path, CDBR_DEFAULT)) == NULL)
	err(EXIT_FAILURE, "cdbr_open");

/*
 * Perform a lookup.  Note that it must be validated that the value
 * corresponds to our key, e.g. pref_match() illustrates the prefix check
 * for the example above where the key is a part of the value as a prefix.
 */
if (cdbr_find(cdb, key, keylen, &data, &len) == 0 && pref_match(data, key)) {
	/* Found .. */
} else {
	/* Not found .. */
}

cdbr_close(cdb);
```

For more details, see cdb(5), cdbr(3) and cdbw(3) manual pages.

## Author ##

This code is derived from software contributed to The NetBSD Foundation
by Joerg Sonnenberger.
