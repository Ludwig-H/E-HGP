# Harnais de coût MEB privé — préparé, non compilé et non mesuré

5 septembre 2026 ; `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=preparation_cout_local`, `public_status=not_claimed`.
Ce dossier n'est ni un résultat de performance ni une intégration produit. GCP non utilisé.

Autorité de conception inchangée : `../v7_meb_dual_budget_cost_plan/PROTOCOL.md`
SHA256 `4a76f875485f39a7b0c5707e53f82c635cc1dfdab73bf595bf8348681b8f9ea7`.
Le gate géométrique c9971f8c et son runner b04dc2a6 sont réutilisés explicitement ;
leur reçu réellement clos `../v7_meb_dual_budget_geometry/run_20260905/receipt.json`
SHA256 `b81d8e480b158710874de230c3485f79d0a42f1cb228e321c750de0f58bed49e`
qualifie la voie **Trace seulement**, pas encore le nominal NoObserver de ce harnais.
Le runner coût rejuge ses six commandes/bruts, la causalité du mutant, ses 38 artefacts,
ses sources avant/après et sa fermeture compilée. Un statut synthétique seul ne suffit pas.

## Réemploi et portée

`cost_harness.cpp` inclut le gate à `main` renommé : aucun appel au main/run historique.
Il réemploie corpus, index, ordres, sentinelles et comparateurs ; le prototype 0645aa00
et les sources F restent inchangés. Le donor n'expose pas de liste de frontières :
son inventaire de 123 appels est porté explicitement, puis les métriques complètes
sont confrontées à `boundaries<false>` hors chrono, sous pin du donor et revue de diff.
Les quatre appels P7/L12 et les **deux** appels P0/L8 conservent Work/statistiques
entre appels. Le triangle de frontière déjà fermé reste distinct des 176 scènes principales.

Le nominal NoObserver est réellement confronté hors chrono à F et au Trace qualifié,
avec toutes statistiques/événements/clé/niveau/support/statut/raison/booléen et Work,
avant puis après mesure. Les reçus antérieurs ne tiennent pas lieu de cette comparaison.
9 216 principaux + 123 frontières + 12 cas q2 répétés donnent 9 347 jobs, 9 351 états
observés et 58 491 appels top-level par bras/passage. Deux chauffes puis sept passages
alternent F→dual / dual→F. Les replis F imbriqués comptent aussi dans le plafond prospectif
de 2 000 000 entrées MEB ; borne structurelle conservatrice totale : 1 692 591.
Les 384 lectures de R sont hors chrono ; q est repris d'un résultat déjà qualifié P0/L=R.

Le temps inclut les resets symétriques, barrières et capture complète par champs.
Cette capture peut coûter beaucoup sur q2 immédiat : ne pas nommer le chiffre obtenu
« helper nu », le soustraire arbitrairement, ni l'extrapoler au CLI Release/la tour.
Une frontière GCC `noinline,noipa`, une indirection protégée et des barrières mémoire
doivent être **constatées dans le désassemblage** ; la source seule ne l'établit pas.
Les causes de replis non observables restent non attribuées ; les strates séparent
n/q de référence, plafonds/états initiaux, terminaux et routes, sans choisir le meilleur P.

## Exécution future, deux autorisations distinctes

Sans `--execute`, `run_cost.py` produit seulement un aperçu sans subprocess ni écritures.
Il n'existe aucun GO de compilation ou de mesure dans cette préparation.

1. Après GO de compilation : `--stage build --execute`, avec
   `--expected-runner-sha256` et `--expected-geometry-receipt-sha256` explicites.
   CPU0, 60 s cumulées, dossier create-only `build_20260905` ; C++20/O2 strict,
   `-fno-lto`, pas MHGP7_TESTING/sanitizer, puis désassemblage épinglé. Aucun lancement du harnais.
2. Après revue du désassemblage et GO de mesure : `--stage measure --execute`,
   les mêmes pins et `--expected-build-receipt-sha256`, `--expected-binary-sha256`,
   `--expected-disassembly-sha256`, `--disassembly-reviewed` explicites.
   CPU6, un thread, fenêtre dédiée confirmée par root, 120 s cumulées et nettoyage compris,
   dossier create-only `run_20260905`. Refus du préflight avant création d'une destination.

Le SHA en argv du C++ est une liaison syntaxique, **pas un lecteur de reçu** :
seul le runner épinglé fournit l'admission effective completed et le délai externe.
Une invocation brute du C++ n'est pas une campagne autorisée/certifiée de ce protocole.
Le build reste `measurement_executed=false` ; un échec est conservé et ne déclenche
ni cap relevé, ni sous-corpus, ni relance. Anciennes destinations jamais écrasées.
Les tableaux bruts de timing et tous les refus restent disponibles ; une campagne
incomplète n'autorise ni ratio choisi ni extrapolation 50k/GPU/dizaines de millions.

## Tests de protocole

`selftest.py` est exclusivement synthétique, avec subprocess interdit ou simulé et fichiers
temporaires isolés ; il ne compile ni ne lance un moteur. Les vérifications Python
n'utilisent pas `assert` comme porte et doivent aussi passer sous `python3 -O`.
La préparation enregistrera ses propres sorties normal/-O sans modifier les sceaux historiques.
Le runner capture le modèle du CPU choisi, les CPUs online et l'affinité avant/effective/après,
sans exposer l'environnement global ; toute dérive du modèle/online ferme un échec.
