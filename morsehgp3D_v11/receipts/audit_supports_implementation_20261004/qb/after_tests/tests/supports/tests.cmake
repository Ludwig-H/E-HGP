# Portes du module supports (tranche S6a de la sortie parametree : Q_b et comptes du lemme G ; l'assemblage attend le
# rattachement de la tour, tranche S3).
mhgp11_add_unit(mhgp11_supports_unit SOURCES supports_test.cpp
                GROUPS square right_triangle growth cube octahedron circle lines mixed shell_bound refusals constants
                       concurrency
                LABELS fast)

# Sonde JSON canonique (Q_b, fermeture, comptes par boule ; agregats sur W_K ; registres de la foret d'ordre K).
add_executable(mhgp11_supports_probe ${CMAKE_CURRENT_LIST_DIR}/supports_probe.cpp)
target_link_libraries(mhgp11_supports_probe PRIVATE mhgp11)
# Fixture 13 (sphere50) : les 84 points entiers de x^2 + y^2 + z^2 = 50 translates de +10 forment une coquille de 84
# sites au-dela du plafond kMaxShell = 24 : refus support_shell_capacity de l'appel entier, aucune ligne de boule.
mhgp11_expect_code(mhgp11_supports_shell_capacity 2 mhgp11_supports_probe --sphere=50,10 --k=2 --all
                   LINE "supports_probe_verdict refus support_shell_capacity" LABELS unit fast)
