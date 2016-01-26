# Vagrant helper

This [vagrant](http://vagrantup.com/) configuration allow you to setup
a complete dev environment for packetgraph.

During vagrant provisioning will
- Build packetgraph project
- Build and run packetgraph tests

This is mainly a tool for dev who wants to quickly setup their dev environment
and start working on packetgraph.

Note that packetgraph tests needs to run a VM so your running hypervisor will need to
support nested virtualization.
