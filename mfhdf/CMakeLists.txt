cmake_minimum_required (VERSION 2.8.10)
PROJECT (HDF4_MFHDF)

#-----------------------------------------------------------------------------
# Option to build HDF4 Tools
#-----------------------------------------------------------------------------
IF (EXISTS "${HDF4_MFHDF_SOURCE_DIR}/dumper" AND IS_DIRECTORY "${HDF4_MFHDF_SOURCE_DIR}/dumper")
  OPTION (HDF4_BUILD_TOOLS  "Build HDF4 Tools" OFF)
  IF (HDF4_BUILD_TOOLS)
    SET (NCGEN_UTILITY 0)
    ADD_SUBDIRECTORY (${HDF4_MFHDF_SOURCE_DIR}/dumper      ${HDF4_MFHDF_BINARY_DIR}/dumper)
    ADD_SUBDIRECTORY (${HDF4_MFHDF_SOURCE_DIR}/hdfimport   ${HDF4_MFHDF_BINARY_DIR}/hdfimport)
    ADD_SUBDIRECTORY (${HDF4_MFHDF_SOURCE_DIR}/hdiff       ${HDF4_MFHDF_BINARY_DIR}/hdiff)
    ADD_SUBDIRECTORY (${HDF4_MFHDF_SOURCE_DIR}/hrepack     ${HDF4_MFHDF_BINARY_DIR}/hrepack)
    ADD_SUBDIRECTORY (${HDF4_MFHDF_SOURCE_DIR}/ncgen       ${HDF4_MFHDF_BINARY_DIR}/ncgen)
    ADD_SUBDIRECTORY (${HDF4_MFHDF_SOURCE_DIR}/ncdump      ${HDF4_MFHDF_BINARY_DIR}/ncdump)
    IF (BUILD_TESTING)
      ADD_SUBDIRECTORY (${HDF4_MFHDF_SOURCE_DIR}/nctest    ${HDF4_MFHDF_BINARY_DIR}/nctest)
    ENDIF (BUILD_TESTING )
  ENDIF (HDF4_BUILD_TOOLS)
ENDIF (EXISTS "${HDF4_MFHDF_SOURCE_DIR}/dumper" AND IS_DIRECTORY "${HDF4_MFHDF_SOURCE_DIR}/dumper")
