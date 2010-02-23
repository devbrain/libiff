set (ea_src ea/ea_io.cpp ea/id.cpp)
set (ea_hdr ea/ea_io.hpp ea/id.hpp)

set (iff_src parser.cpp structure.cpp)
set (iff_hdr parser.hpp structure.hpp)


add_library (iff_ea ${ea_src} ${ea_hdr})
add_library (iff_core ${iff_src} ${iff_hdr})


