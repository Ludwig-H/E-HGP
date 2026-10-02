# Portes du module cloud.
mhgp11_add_unit(mhgp11_cloud_unit SOURCES cloud_test.cpp
                GROUPS morton width sizes refusals fixture judge permutation renumbering budget
                LABELS fast)

# Penurie de memoire injectee : operator new remplace dans cet executable seulement.
mhgp11_add_unit(mhgp11_cloud_fault SOURCES cloud_fault.cpp GROUPS starvation LABELS fast)

# Refus a la compilation : une largeur de coordonnees hors bornes n'est pas representable.
mhgp11_expect_compile_failure(mhgp11_cloud_width_unrepresentable SOURCE width_probe.cpp
                              TOKEN "private" OPTIONS -DMHGP11_NEGATIVE=1 LABELS unit fast)
