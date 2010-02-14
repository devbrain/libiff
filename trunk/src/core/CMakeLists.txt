set (ea_src ea/ea_io.cpp ea/id.cpp)
set (ea_hdr ea/ea_io.hpp ea/id.hpp)

add_library (iff_ea ${ea_src} ${ea_hdr})
