

# build_lib_example(
#         NAME ns3ai_ore_msg_stru
#         SOURCE_FILES use-msg/ORE-env.cc
#         LIBRARIES_TO_LINK 
#                 ${libai}
#                 ${liblte}
# )

build_lib_example(
        NAME nr-v2x-ore
        SOURCE_FILES nr-v2x-ore.cc
                use-msg/ORE-env.cc
                use-msg/ai-nr-sl-ue-mac-scheduler-simple.cc
                use-msg/ai-nr-ue-mac.cc
        HEADER_FILES    
                use-msg/ai-nr-sl-ue-mac-scheduler-simple.h
                use-msg/ai-nr-ue-mac.h
        LIBRARIES_TO_LINK 
                ${libai} 
                ${libcore}
                ${libnr} 
                ${libstats}
                ${libflow-monitor}
                ${SQLite3_LIBRARIES}
)

pybind11_add_module(ns3ai_ore_py_stru use-msg/ORE-env_py.cc)
set_target_properties(ns3ai_ore_py_stru PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)

# Build Python binding library along with C++ library
# add_dependencies(ns3ai_ore_msg_stru ns3ai_ore_py_stru)
add_dependencies(nr-v2x-ore ns3ai_ore_py_stru)


