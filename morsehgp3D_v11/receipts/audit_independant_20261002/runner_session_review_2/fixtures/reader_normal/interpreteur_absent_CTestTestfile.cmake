add_test(mhgp11_target "/usr/bin/cmake" "-DCMD=/tmp/mhgp11-runner-session-review-2.jp9tcot2/reader_normal/campaign/interpreteur_absent/src/tests/program" "-DEXPECTED=0" "-DNARGS=0" "-P" "/tmp/mhgp11-runner-session-review-2.jp9tcot2/reader_normal/campaign/interpreteur_absent/src/cmake/run_expect.cmake")
set_tests_properties(mhgp11_target PROPERTIES TIMEOUT 5)
