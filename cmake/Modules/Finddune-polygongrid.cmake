include (OpmPackage)
# find dune-alugrid when build with autotools (deprecated in dune 2.4 and removed after dune 3.0
find_opm_package (
  # module name
  "dune-polygongrid"

  # dependencies
  # TODO: we should probe for all the HAVE_* values listed below;
  # however, we don't actually use them in our implementation, so
  # we just include them to forward here in case anyone else does
  "CXX11Features REQUIRED;
   dune-grid REQUIRED;
  "
  # header to search for
  "dune/polygongrid/grid.hh"

  # library to search for
  ""

  # defines to be added to compilations
  ""

  # test program
"#include <dune/polygongrid/grid.hh>
int main (void) {
   return 0;
}
"
  # config variables
  "HAVE_DUNE_POLYGONGRID
  ")


# make version number available in config.h
include (UseDuneVer)
find_dune_version ("dune" "polygongrid")
