# Suivi des verrous — preuves de l'audit du 2 octobre 2026

Entrée courante : [note de l'auditeur](../../../audits/AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).
Ce dossier conserve une capture et quatre revues bornées ; il n'est ni une
campagne produit, ni une qualification G4. Aucun build ou CTest lancé.

| Objet | Preuve et portée |
| --- | --- |
| FULL → cover / MR₂-bord | [Témoin exact de trois points](points_review/README.md), quatre contrôles MR, transformations, normal/−O identiques. |
| Q1 : tour et mémos | [Contrelecture des théorèmes](tower_review/README.md), trois petites fixtures Fraction ; domaine ouvert/fermé du mémo explicité. |
| F3/F4/F6 | [Preuve et garde du seuil](numeric_review/README.md), calculs Fraction ; aucune exécution native FENV. |
| Socle corrigé | [Relecture et contrôles Python](foundation_followup/FOLLOWUP.md), nouvelle fixture de faux signal fournie pour G4, non exécutée. |

`snapshot.json` décrit les 72 fichiers capturés le 2 octobre à
08:30:04.919611 UTC sur la base `986f75799`, y compris des sources alors non
suivies. `snapshot/` en conserve les octets. Les anciennes notes qui s'y
trouvent sont des entrées historiques, jamais l'état courant.
`commit_reconciliation.json` compare leurs empreintes au commit publié
`2f9eb838a` : 62 identiques ; changements de documentation et absences
distingués. Aucun transfert de résultat aux fichiers différents.

Les commandes originales et leurs codes restent dans chaque sous-dossier.
Les chemins absolus de provenance désignent le lieu de capture ; le lecteur
de clôture ne les utilise pas. Il vérifie les fichiers locaux par chemins
relatifs, les 72 sources, la note conservée au moment de la clôture et les
quatre paires normal/−O : trois identiques octet pour octet, celle du lecteur
de portes identique hormis son champ explicite `mode` :

```
python3 -B recheck.py
python3 -B -O recheck.py
```

`closure_manifest.json` scelle les artefacts et `current_note_at_close.md`
conserve les octets de la note publiée. La note dans `audits/` peut ainsi
être tenue à jour sans réécrire l'histoire du reçu. Les résultats du lecteur
sont dans `closure_normal.json` et `closure_optimized.json` ; le lecteur
n'exécute aucun des scripts ni aucune commande produit.
