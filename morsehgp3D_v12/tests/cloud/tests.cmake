# Portes du module cloud.
mhgp12_add_unit(mhgp12_cloud_unit SOURCES cloud_test.cpp
                GROUPS morton width sizes refusals fixture judge permutation renumbering budget ownership
                       refusal_priority shared_budget
                LABELS fast)

# Penurie de memoire injectee : operator new remplace dans cet executable seulement.
mhgp12_add_unit(mhgp12_cloud_fault SOURCES cloud_fault.cpp GROUPS starvation LABELS fast)

# Refus a la compilation : une largeur de coordonnees hors bornes n'est pas representable.
mhgp12_expect_compile_failure(mhgp12_cloud_width_unrepresentable SOURCE width_probe.cpp
                              TOKEN "private" OPTIONS -DMHGP12_NEGATIVE=1 LABELS unit fast)

mhgp12_expect_compile_failure(mhgp12_cloud_factory_only SOURCE immutability_probe.cpp
                              TOKEN "private" OPTIONS -DMHGP12_NEGATIVE=1 LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_cloud_const_views SOURCE immutability_probe.cpp
                              TOKEN mhgp12_cloud_mutable_view_interdite OPTIONS -DMHGP12_NEGATIVE=2 LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_cloud_copy_refused SOURCE immutability_probe.cpp
                              TOKEN "deleted" OPTIONS -DMHGP12_NEGATIVE=3 LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_cloud_assignment_refused SOURCE immutability_probe.cpp
                              TOKEN "deleted" OPTIONS -DMHGP12_NEGATIVE=4 LABELS unit fast)
