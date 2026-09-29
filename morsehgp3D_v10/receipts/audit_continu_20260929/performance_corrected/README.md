# Capture légère du contre-audit des prototypes CPU

Audit du 29 septembre 2026, local, hors registre. Les fichiers `observed/` sont des **copies de journaux et
sources d'autres acteurs**, pas des exécutions lancées par ce contre-audit. Aucun build, binaire ni nuage
massif n'est recopié ; aucun processus tiers n'est relancé ou interrompu.

La note courante est [CONTRE_AUDIT_PROTO_CPU_20260929.md](../../../audits/audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md).

- `HASHES_ORIGINAUX_OUVERTURE.sha256` : 82 fichiers lus, figés au début de la capture à 21:44:15 UTC.
- `HASHES_ORIGINAUX_FERMETURE.sha256` : mêmes cibles revérifiées après copie et interprétation.
  À 21:52:29 UTC, **81 inchangées sur 82**, aucune modifiée ; seule la source temporaire du mutant a disparu.
  `sha256sum` garde son code1 dans `CAPTURE.json` et son erreur dans `FERMETURE_ERRORS.txt`. La copie du mutant
  avait été faite avant disparition et garde le hash d'ouverture. Aucun 82/82 LIVE n'est revendiqué.
- `HASHES_BINAIRES_*.sha256` : empreintes ponctuelles des neuf exécutables concernés, sans copie des binaires.
- `HASHES_CAPTURE.sha256` : intégrité des fichiers de cette capture, hors ce manifeste lui-même.
- `observed/` : preuves sélectionnées. Les tableaux g101/g202/g303 conservent les **dix timeout** ;
  le dénominateur est **940 cas entièrement conclusifs**, jamais 950 passes.
- `GENERATOR_PARITE_EXTRAIT.txt`, `observed/feuille/src/.../leaf.hpp`,
  `observed/M3_haut_strict.leaf.hpp` : preuve que le mutant survivant de borne haute est équivalent
  par parité, sous la précondition produit `S.x=64S.p`.
- `TOUR_MESURES_EXTRAITES.csv` : 280 observations projetées depuis neuf JSONL d'origine dont les hashes sont
  conservés. Colonnes utiles et digests complets, sans recopier leurs détails volumineux.
- `export_tour_observed.py` : extraction en lecture seule, vers stdout. Ne lance aucun moteur.
- `VALIDATION_EXTRACTION_TOUR.json` : 280 lignes, dénominateurs par série/fils/variante et digest unique de
  chaque série ; vérification en lecture du CSV, sans nouvelle exécution HGP.
- `TOUR_RESUMES_RECALCULES.txt` : sorties des scripts de résumé existants, rejoués uniquement sur les JSONL
  déjà produits. Leur exécution n'est pas une nouvelle mesure de performance.
- `PROCESSUS_OBSERVES.txt` et `J3_CLANG_VIVANT.txt` : instantané ciblé par exécutables ou journaux sous
  v10-perf/v10-fixes, sans inventaire global des arguments. Le fuzzer Clang g505 vivant au dernier contrôle
  n'est pas inclus dans les séries closes. Ces instantanés sont des observations datées, pas une autorité LIVE.
- `J3_CLANG_CLOTURE.txt` et `HASHES_G505_*.sha256` : actualisation à 21:53:46 UTC, PID708851 absent,
  120 cas g505 désormais conclusifs, zéro délai/écart. Série ajoutée séparément ; l'instantané vivant reste
  historique et le dénominateur g101/g202/g303 reste 940 + dix délais.

Les sources J3, frontière et tour concernent des copies locales de prototypes. Les hashes de fermeture et
les vérifications des anciens manifestes ne créent pas rétroactivement une qualification d'exécution
avant/après pour chaque campagne historique. Ils établissent les octets effectivement relus par cet audit.
Aucune nouvelle mesure G4, aucun résultat GPU d'exécution, aucune borne globale, ni FULL K5 en 100 ms acquis.
