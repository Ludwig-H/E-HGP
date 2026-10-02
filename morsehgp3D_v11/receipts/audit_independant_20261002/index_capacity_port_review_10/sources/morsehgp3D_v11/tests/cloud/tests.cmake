# Portes du module cloud.
mhgp11_add_unit(mhgp11_cloud_unit SOURCES cloud_test.cpp
                GROUPS morton width sizes refusals fixture judge permutation renumbering budget ownership
                       refusal_priority shared_budget
                LABELS fast)

# Penurie de memoire injectee : operator new remplace dans cet executable seulement.
mhgp11_add_unit(mhgp11_cloud_fault SOURCES cloud_fault.cpp GROUPS starvation LABELS fast)

# Refus a la compilation : une largeur de coordonnees hors bornes n'est pas representable.
mhgp11_expect_compile_failure(mhgp11_cloud_width_unrepresentable SOURCE width_probe.cpp
                              TOKEN "private" OPTIONS -DMHGP11_NEGATIVE=1 LABELS unit fast)

mhgp11_expect_compile_failure(mhgp11_cloud_factory_only SOURCE immutability_probe.cpp
                              TOKEN "private" OPTIONS -DMHGP11_NEGATIVE=1 LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_cloud_const_views SOURCE immutability_probe.cpp
                              TOKEN mhgp11_cloud_mutable_view_interdite OPTIONS -DMHGP11_NEGATIVE=2 LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_cloud_copy_refused SOURCE immutability_probe.cpp
                              TOKEN "deleted" OPTIONS -DMHGP11_NEGATIVE=3 LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_cloud_assignment_refused SOURCE immutability_probe.cpp
                              TOKEN "deleted" OPTIONS -DMHGP11_NEGATIVE=4 LABELS unit fast)
