Explanation of client API
 Client may include the following files.
 
Metadata creation API
=====================
 cfg_mgr_simple_descriptor.h
 cfg_mgr_composite_descriptor.h
 cfg_mgr_contained_aggregate.h
 cfg_mgr_owned_aggregate.h
 cfg_mgr_types.h - Common types used by client-implemented set and print functions.
 util/*.h - various set, setdef and print functions for simple items -- we keep pointers to them in ROMable metadata.
 
 cfg_mgr_printf???
 cfg_mgr_prt_hexstr.h should be moved to export, since it's part of lib but also a client util?
 
Command-handling API
====================
 cfg_mgr_interface.h
  - Instantiate Config_manager_interface, passing it a descriptor reference.
  - Pass commands to it.
  - Get config to it, i.e. access the RAM that it allocates and manages.
 util/cfg_mgr_strtok.h
  - Help 'chop' input command strings into tokens.
