add_executable ( Points points.cpp )

target_link_libraries ( Points
        ${DEFAULT_LIBS}
 )

#target_compile_features(Points PRIVATE cxx_range_for)
