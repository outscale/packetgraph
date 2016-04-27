### Portable Property container object library

The proplib library provides an abstract interface for creating and
manipulating property lists.  Property lists have object types for
boolean values, opaque data, numbers, and strings.  Structure is provided
by the array and dictionary collection types.

Property lists can be passed across protection boundaries by translating
them to an external representation.  This external representation is an
XML document whose format is described by the following DTD:

  http://www.apple.com/DTDs/PropertyList-1.0.dtd

### About this version

The portable proplib library is derived from the original implementation that
appeared on NetBSD, and contains some changes:

- New interfaces have been added to the API, yet it's ABI compatible.

- The original NetBSD implementation is using base 16, not base 10 for the
external representation of unsigned numbers, but portable proplib doesn't have
this difference, it is using base 10 as Apple does.

- This implementation supports reading from gzip compressed plist files (and
writing as well) thanks to the zlib library.

- If platform supports fdatasync, portableproplib uses this rather than fsync.

- It can be built through the GNU autotools (configure/make).

-  This implementation is only meant to be used in user space interfaces
(unlike NetBSD's).

The portable proplib library is not fully compatible with Apple property lists,
there are some exceptions:

- Property lists in binary format are not supported.
- The date and real elements are not supported.
- The external representation will never be negative in integer elements.

It should be mentioned that NetBSD's proplib code is a free, clean room
implementation based in the specifications available for Mac OS X, written
originally by Jason R. Thorpe.
