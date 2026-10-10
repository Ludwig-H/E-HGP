# Repli mémoire : avis préalable au chantier C

10 octobre 2026, source `fbd5923a8a892e1dbab8117f0e62d04f1b432d2a`,
produit B3-K. Relecture statique des API ; aucun nouveau code, moteur ou test
natif. FULL u21, exploration v12 hors registre, `public_status=not_claimed`.
Le développeur annonce ce repli dans son reçu MES-B1o ; son implantation
future n'est pas présumée ici.

Le séquentiel peut réduire la mémoire simultanée : `stage.cpp:165–169`
prévoit la plus grande table G, là où la Session recouverte conserve les
tables des ordres. Cela ne prouve pas que les grandes scènes refusées tiendront.

1. **Nommer le déclencheur.** `pipeline.cpp:203–218` réunit `budget.admit`,
   allocations des espaces et construction des tables. Un `memory_budget`
   ne distingue pas ces trois causes. Replier uniquement après refus de la
   borne est le périmètre minimal. Replier après allocation refusée est
   possible, mais élargit la politique à déclarer et tester. Ne pas masquer
   invariants, entrées invalides ou `pool_busy`.
2. **Rendre l'état abandonné.** `open_session` possède déjà points exacts,
   `Resolution`, `BuildState` et `Pipeline` ; une admission partielle ajoute
   espaces et tables. Avant un nouveau `resolve_tower`, détruire ce
   `SessionRun`, garder index/catalogue/Pool et vérifier le retour de `used`
   à sa valeur avant ouverture. Le cache peut garder des blocs inactifs :
   aucune baisse de RSS n'est promise.
3. **Résoudre avant les forêts.** Après `open_stage`, les cibles des
   représentants restent inachevées (`stage.hpp:63–66`). Le chemin public
   enchaîne `resolve_tower`, puis `build_forests`, en gardant la résolution
   vivante pour ses vues empruntées. Réutiliser l'ouverture demanderait un
   raccord explicite avec `resolve_orders` et sa propre admission ; la borne
   `orders_bytes` est aujourd'hui privée à `stage.cpp`.
4. **Conserver les garanties.** Même budget et admissions du séquentiel,
   aucun résultat partiel publié. Couvrir trois issues effectivement
   parcourues : recouvert accepté ; refus recouvert/succès séquentiel ;
   deux refus. Comparer FUL1 valide, cibles, forêts complètes, branches R et
   historiques ; vérifier restitution et répétition, W1/W3, cache nul/actif,
   chaîne ON/OFF. Un repli après allocation peut légitimement faire réussir
   une injection : l'actuelle assertion « toute allocation injectée refuse »
   doit alors être adaptée explicitement, sans supprimer le contrôle des
   fuites ni la couverture de la faute.
5. **Mesurer le chemin exécuté.** Tentative, destruction et repli restent
   dans le mur et le pic publiés. Déclarer la route et adapter les lecteurs ;
   ne pas présenter des diagnostics séquentiels comme un recouvrement.

[CST0244](a6c_admission_proposition/README.md) demeure distinct : le supplément
N inconditionnel peut provoquer un repli inutile quand la chaîne est OFF.
Sa correction et les portes internes exact/moins-un restent à qualifier.
Les [cinq refus MES-B1o](mesb1o_temps/README.md) ne sont pas encore localisés :
un repli de la tour ne constitue pas d'avance leur correction.
