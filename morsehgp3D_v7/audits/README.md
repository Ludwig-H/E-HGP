# Audits courants de MorseHGP3D v7

Entrée maintenue le 5 septembre 2026. Toutes les écritures de l'auditeur
restent dans ce dossier ; le constructeur réalise les changements produit.

Commencer par la [synthèse indépendante](AUDIT_INDEPENDANT_20260904.md)
et l'[état courant](ETAT_COURANT.md), puis consulter :

- [Échanges avec le constructeur](DIALOGUE_COURANT.md) : acquis désormais levés et prochaines fermetures concrètes.
- [MEB différée](AUDIT_MEB_DIFFEREE_20260905.md) : preuve locale, oracle rationnel indépendant, budgets et mutants.
- [Addendum q2 E](ADDENDUM_MEB_Q2_E_20260905.md) : delta ultérieur jugé localement, sans transfert de la suite D.
- [Index Morton/Karras](AUDIT_INDEX_20260905.md) : preuve de partition et oracle de trie indépendant.
- [Raccord index/front](AUDIT_RACCORD_INDEX_FRONT_20260905.md) : populations, antichaînes et permutation du cover.
- [Garde d'arrondi](AUDIT_ARRONDI_20260905.md) : quatre modes, replis et objets effectivement contrôlés.
- [Qualification des reçus D](AUDIT_QUALIFICATION_20260905.md) : intégrité, tests, sources, binaires et autorité Boost.
- [Reconstruction indépendante D](receipts_20260905/release/summary.json) : build neuf et CTest locaux du présent audit.
- [Mathématiques et hiérarchie](AUDIT_MATHEMATIQUE_20260904.md) et [couverture S1](S1_COURANT.md) : composition conditionnelle et raccord des primitives.
- [Interfaces](AUDIT_INTERFACES_20260904.md), [mode mono](MONO_COURANT.md) et [census](CENSUS_AXIS_COURANT.md) : contrats et contre-fixtures conservés.
- [Arithmétique des lanes](ARITHMETIQUE_LANES_COURANTE.md) et [entiers larges](ARITHMETIQUE_LARGE_COURANTE.md) : bornes locales et portes compilées.
- [Résidence](AUDIT_RESIDENCE_20260904.md) et [retour mémoire](RETOUR_MEMOIRE_COURANT.md) : coûts et propositions ciblées encore applicables.
- [Validation courante](receipts_20260905/validation_current.json) : sources épinglées et autorités des contrôles.

Les rapports courants retirent les objections corrigées. Les reçus
historiques conservent leurs résultats, y compris échecs et refus ; ils ne
sont pas transformés en résultats du dernier code. Les copies de sources,
binaires et temporaires restent sous `.work*`, ignorés par Git.

Avant de réutiliser les conclusions, contrôler les sources depuis la racine :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/verify_current.py
```

Le code 1 demande une actualisation des octets indiqués ; le code 2 signale
un manifeste invalide. Le code 0 confirme uniquement la fraîcheur des pins,
sans réexécuter les tests ni promouvoir le produit. Les contrôles CTest
locaux et les runs GitHub sont des autorités distinctes.

Statut public : `not_claimed`. GCP non utilisé.

Le [recueil des reçus](receipts_20260905/README.md) donne les commandes
de reproduction et le périmètre exact. E q2 a commencé dans le worktree
après le run D : le code de fraîcheur 1 sur les quatre fichiers concernés
est attendu ; le présent commit d'audit ne publie pas ces changements produit.
