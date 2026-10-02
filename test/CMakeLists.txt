configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

set(CIRCC_TEST_DEPENDS
  circc
  FileCheck
  not
)

add_lit_testsuite(check-circc "Running the circc regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CIRCC_TEST_DEPENDS}
)

add_lit_testsuites(CIRCC ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CIRCC_TEST_DEPENDS}
)
