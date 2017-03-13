/* Legit packet
Frame 16: 86 bytes on wire (688 bits), 86 bytes captured (688 bits)
Ethernet II, Src: RealtekU_12:34:02 (52:54:00:12:34:02),
	     Dst: RealtekU_12:34:01 (52:54:00:12:34:01)
    Destination: RealtekU_12:34:01 (52:54:00:12:34:01)
    Source: RealtekU_12:34:02 (52:54:00:12:34:02)
    Type: IPv6 (0x86dd)
Internet Protocol Version 6, Src: 2001:db8:2000:aff0::2,
			     Dst: 2001:db8:2000:aff0::1
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... =
	Traffic class: 0x00 (DSCP: CS0, ECN: Not-ECT)
    .... .... .... 0000 0000 0000 0000 0000 = Flow label: 0x00000
    Payload length: 32
    Next header: ICMPv6 (58)
    Hop limit: 255
    Source: 2001:db8:2000:aff0::2
    Destination: 2001:db8:2000:aff0::1
    [Source GeoIP: Unknown]
    [Destination GeoIP: Unknown]
Internet Control Message Protocol v6
    Type: Neighbor Advertisement (136)
    Code: 0
    Checksum: 0x9638 [correct]
    [Checksum Status: Good]
    Flags: 0x60000000
	0... .... .... .... .... .... .... .... = Router: Not set
	.1.. .... .... .... .... .... .... .... = Solicited: Set
	..1. .... .... .... .... .... .... .... = Override: Set
	...0 0000 0000 0000 0000 0000 0000 0000 = Reserved: 0
    Target Address: 2001:db8:2000:aff0::2
    ICMPv6 Option (Target link-layer address : 52:54:00:12:34:02)
	Type: Target link-layer address (2)
	Length: 1 (8 bytes)
	Link-layer address: RealtekU_12:34:02 (52:54:00:12:34:02)
*/
static const unsigned char pkt0[86] = {
0x52, 0x54, 0x00, 0x12, 0x34, 0x01, 0x52, 0x54, /* RT..4.RT */
0x00, 0x12, 0x34, 0x02, 0x86, 0xdd, 0x60, 0x00, /* ..4...`. */
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x20, 0x01, /* ... :. . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x01, /* ...... . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x88, 0x00, /* ........ */
0x96, 0x38, 0x60, 0x00, 0x00, 0x00, 0x20, 0x01, /* .8`... . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x01, /* ........ */
0x52, 0x54, 0x00, 0x12, 0x34, 0x02              /* RT..4. */
};

/* Non legit packet, target address is not the right one
Frame 16: 86 bytes on wire (688 bits), 86 bytes captured (688 bits)
Ethernet II, Src: RealtekU_12:34:02 (52:54:00:12:34:02),
	     Dst: RealtekU_12:34:01 (52:54:00:12:34:01)
    Destination: RealtekU_12:34:01 (52:54:00:12:34:01)
    Source: RealtekU_12:34:02 (52:54:00:12:34:02)
    Type: IPv6 (0x86dd)
Internet Protocol Version 6, Src: 2001:db8:2000:aff0::2,
			     Dst: 2001:db8:2000:aff0::1
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... =
	Traffic class: 0x00 (DSCP: CS0, ECN: Not-ECT)
    .... .... .... 0000 0000 0000 0000 0000 = Flow label: 0x00000
    Payload length: 32
    Next header: ICMPv6 (58)
    Hop limit: 255
    Source: 2001:db8:2000:aff0::2
    Destination: 2001:db8:2000:aff0::1
    [Source GeoIP: Unknown]
    [Destination GeoIP: Unknown]
Internet Control Message Protocol v6
    Type: Neighbor Advertisement (136)
    Code: 0
    Checksum: 0x9638 [correct]
    [Checksum Status: Good]
    Flags: 0x60000000
	0... .... .... .... .... .... .... .... = Router: Not set
	.1.. .... .... .... .... .... .... .... = Solicited: Set
	..1. .... .... .... .... .... .... .... = Override: Set
	...0 0000 0000 0000 0000 0000 0000 0000 = Reserved: 0
    Target Address: 2001:db8:2000:aff0::06 <--- BAD
    ICMPv6 Option (Target link-layer address : 52:54:00:12:34:02)
	Type: Target link-layer address (2)
	Length: 1 (8 bytes)
	Link-layer address: RealtekU_12:34:02 (52:54:00:12:34:02)
*/
static const unsigned char pkt1[86] = {
0x52, 0x54, 0x00, 0x12, 0x34, 0x01, 0x52, 0x54, /* RT..4.RT */
0x00, 0x12, 0x34, 0x02, 0x86, 0xdd, 0x60, 0x00, /* ..4...`. */
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x20, 0x01, /* ... :. . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x01, /* ...... . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x88, 0x00, /* ........ */
0x96, 0x38, 0x60, 0x00, 0x00, 0x00, 0x20, 0x01, /* .8`... . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x02, 0x01, /* ........ */
0x52, 0x54, 0x00, 0x12, 0x34, 0x02              /* RT..4. */
};

/* Bad Target link-layer address
Frame 16: 86 bytes on wire (688 bits), 86 bytes captured (688 bits)
Ethernet II, Src: RealtekU_12:34:02 (52:54:00:12:34:02),
	     Dst: RealtekU_12:34:01 (52:54:00:12:34:01)
    Destination: RealtekU_12:34:01 (52:54:00:12:34:01)
    Source: RealtekU_12:34:02 (52:54:00:12:34:02)
    Type: IPv6 (0x86dd)
Internet Protocol Version 6, Src: 2001:db8:2000:aff0::2,
			     Dst: 2001:db8:2000:aff0::1
    0110 .... = Version: 6
    .... 0000 0000 .... .... .... .... .... =
	Traffic class: 0x00 (DSCP: CS0, ECN: Not-ECT)
    .... .... .... 0000 0000 0000 0000 0000 = Flow label: 0x00000
    Payload length: 32
    Next header: ICMPv6 (58)
    Hop limit: 255
    Source: 2001:db8:2000:aff0::2
    Destination: 2001:db8:2000:aff0::1
    [Source GeoIP: Unknown]
    [Destination GeoIP: Unknown]
Internet Control Message Protocol v6
    Type: Neighbor Advertisement (136)
    Code: 0
    Checksum: 0x9638 [correct]
    [Checksum Status: Good]
    Flags: 0x60000000
	0... .... .... .... .... .... .... .... = Router: Not set
	.1.. .... .... .... .... .... .... .... = Solicited: Set
	..1. .... .... .... .... .... .... .... = Override: Set
	...0 0000 0000 0000 0000 0000 0000 0000 = Reserved: 0
    Target Address: 2001:db8:2000:aff0::2
    ICMPv6 Option (Target link-layer address : 52:54:00:12:34:06)
	Type: Target link-layer address (2)
	Length: 1 (8 bytes)
	Link-layer address: RealtekU_12:34:06 (52:54:00:12:34:06)
*/
static const unsigned char pkt2[86] = {
0x52, 0x54, 0x00, 0x12, 0x34, 0x01, 0x52, 0x54, /* RT..4.RT */
0x00, 0x12, 0x34, 0x02, 0x86, 0xdd, 0x60, 0x00, /* ..4...`. */
0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0x20, 0x01, /* ... :. . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x01, /* ...... . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x88, 0x00, /* ........ */
0x96, 0x38, 0x60, 0x00, 0x00, 0x00, 0x20, 0x01, /* .8`... . */
0x0d, 0xb8, 0x20, 0x00, 0xaf, 0xf0, 0x00, 0x00, /* .. ..... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x01, /* ........ */
0x52, 0x54, 0x00, 0x12, 0x34, 0x06              /* RT..4. */
};
