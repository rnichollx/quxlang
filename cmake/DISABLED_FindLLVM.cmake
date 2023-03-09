include(FindPackageHandleStandardArgs)

#find_package_handle_standard_args(LLVM)


add_library(LLVM::LLVM INTERFACE IMPORTED)

file(GLOB LLVM_LIBS "${LLVM_ROOT}/lib/libLLVM*.a")

set(LLVM_INCLUDE_DIRS "${LLVM_ROOT}/include" )

message(STATUS "LLVM_LIBS: ${LLVM_LIBS}")
foreach(LLVM_LIB ${LLVM_LIBS})
    message(STATUS "Found LLVM library: ${LLVM_LIB}")
    get_filename_component(LLVM_LIB_NAME ${LLVM_LIB} NAME)
    message(STATUS "Found LLVM library name: ${LLVM_LIB_NAME}")
    #example names:
    # libLLVMX86Disassembler.a
    # libLLVMX86Info.a

    # First, strip libLLVM from the front
    string(REGEX REPLACE "^libLLVM" "" LLVM_LIB_NAME ${LLVM_LIB_NAME})

    # Next, strip .a from the end
    string(REGEX REPLACE "\\.a$" "" LLVM_LIB_NAME ${LLVM_LIB_NAME})

    # Now, we create a target for this archive
    add_library(LLVM::${LLVM_LIB_NAME} STATIC IMPORTED)
    set_target_properties(LLVM::${LLVM_LIB_NAME} PROPERTIES IMPORTED_LOCATION ${LLVM_LIB})

    # Finally, we add it to the list of LLVM libraries
    list(APPEND LLVM_LIBRARIES LLVM::${LLVM_LIB_NAME})

    # And add as a link dependency to the main LLVM target

endforeach()

# Make a string from LLVM_LIBRARIES that replaces the ; with ,
string(REPLACE ";" "," LLVM_LIBRARIES_STR "${LLVM_LIBS}")

target_link_libraries(LLVM::LLVM INTERFACE "$<LINK_GROUP:RESCAN,${LLVM_LIBRARIES_STR}>")




target_include_directories(LLVM::LLVM INTERFACE ${LLVM_INCLUDE_DIRS})

set_target_properties(LLVM::LLVM PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${LLVM_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "${LLVM_LIBRARIES}"
  INTERFACE_COMPILE_DEFINITIONS "${LLVM_DEFINITIONS}"
)
