Butterfly prototol
==================

# Introduction

Butterfly interactions go through it's API.
All actions you should performs are formalized in a Request which should
always responde a Reply.

# Protocol

To formalise it's protocol in a standard way, Butterfly use
[Protocol Buffers](https://developers.google.com/protocol-buffers/).

All messages sent throuth the API are sent using the 'Messages' message
described in 'message.proto'.

For documentation on protocol content and parameters, please read .proto files

# Transport

Message transport is done using [ZeroMQ](http://zeromq.org/).

Requests and responses are done using ZeroMQ REQ / REP sockets.

Content of requests and responses are just protobuf data.
Note that we don't use any ZMQ multipart feature.

ZeroMQ endpoint is configured to be a TCP binding by default but user can
specify whatever he wants supported by ZeroMQ.

# Upgrading

We must never break protocol when upgrading a butterfly.
This is important for large upgrades. Here is a common upgrade scenario:

1. Butterflies and clients are speaking using protocol n
2. All butterflies are upgraded
3. Clients can still speak with butterflies using protcol n
4. Clients are upgraded and clients speaks to butterflies using protocol n+1

# Evolving protocol

A new version of the protocol must be at least compatible with the previous
one. Protocol can evolve, and there are some rules to not break old versions:

- Never change parameter identifiers
- Don't delete existing parameter
- When making an evolution of an existing message (like MessageV0), don't add
  a 'required' parameter.
- If evolution is too important or needs 'required' parameters, create a new
  message (like MessageV1).
- Whatever change is made on the protocol, you MUST always increment de
  revision number and add a few notes in the changelog.

Note that revision number is just an identifier of the current used protocol.
It does not mean anything except tracing the different evolutions of the
protocol (like a document has several revisions).

To increment revision number, just edit 'revision.h' file.

# Changelog

## Revision 0

First release of the protocol:
- MessageV0 is created, please refer to 'message_0.proto' file


