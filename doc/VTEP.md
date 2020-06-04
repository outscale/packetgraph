  # VTEP Brick

  ## Introduction

  The VTEP brick is intended to allow us to use the VXLAN protocol.<br>
  The VXLAN protocol is allowing us to create tunnels between multpiple LAN through another network.<br>

  ## VXLAN protocol

  THe VXLAN (Virtual Extensible LAN) protocol work by encapsulating packets before sending them through another network.<br>
  It's all in the 2 OSI layer.

  ## Usage

  Our typical use case is making tunnels between virtual networks (or packetgraph's graphs) through the host's network via a NIC brick.<br>
  When creating the brick, we tell her which side is the output (the tunnel's network), either East or West.<br>
  This brick has an empty poll function because the only way to make packets going through ot is by bursting from an input/output of the graph.<br>

  Example use case: making a tunnel between virtual networks 1 and 2 through the host's network.<br>
  (NIC brick are described in the [NIC section](NIC.md))<br>
  For a better link description between bricks, see [packetgraph's brick concept.](BRICK_CONCEPT.md)<br>
  ```
                           Virtual Network 1          |   Host's network   |           Virtual Network 2
                                                      |                    |
                                                      |                    |
                            |            |            |                    |            |            |
    Virtual Network  1  <---| +--------+ |       +---------+          +---------+       | +--------+ |---> Virtual Network  1
    Virtual Network ... <---|-|  VTEP  |-|------>|   NIC   |----------|   NIC   |<------|-|  VTEP  |-|---> Virtual Network ...
    Virtual Network  n  <---| +--------+ |       +---------+          +---------+       | +--------+ |---> Virtual Network  n
                            |            |            |                    |            |            |
                        West Side    East Side        |                    |        West Side    East Side
                                                      |                    |
                                                      |                    |


  ```
  Using the previous shema, it will be like if VN1 and VN2 were the same network.<br>

  # Output redirection
  The VTEP brick features her own switch. It is based on the VNI of each packet (Virtual Network Identifier).<br>
  One vtep can lead to multiple virtual networks, each one identified by its own VNI.<br>
  On a virtual network, there could be any kind of packetgraph brick, and why not another VTEP? If we want to encapsulate one more time the encapsulated network...<br>
