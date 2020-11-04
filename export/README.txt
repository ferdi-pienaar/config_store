Explanation of client API
 Client may include the following files.
 
Metadata creation API
==========================
 export/cfg_mgr_simple_descriptor.h
 export/cfg_mgr_composite_descriptor.h
 export/cfg_mgr_contained_aggregate.h
 export/cfg_mgr_owned_aggregate.h
 src/cfg_mgr_setdef_null.h xxx move it to utils, since it is not used by cfg_mgr, only by its clients.
 util - various set and print functions for simple items -- we keep pointers to them in metadata.
 export/cfg_mgr_types.h - Common types used by client-implemented set and print functions.
 
 Move util .h files to export/util?
 
 cfg_mgr_printf???
 
Command-handling API
====================
 src/nvram.h
  - Instantiate nvram. Having client inject this to Config_manager
    simplifies testing, since it allows test to inject a spy. xxx move to export, since it is exported.
 export/cfg_mgr_interface.h
  - Instantiate Config_manager, passing it a descriptor reference and nvram reference.
  - Pass commands to it.
  - Get config to it, i.e. access the RAM that it allocates and manages.
 util/cfg_mgr_strtok.h
  - Help 'chop' input command strings into tokens.
