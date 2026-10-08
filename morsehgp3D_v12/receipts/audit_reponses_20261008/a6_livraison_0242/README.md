# A6 livré : raccord de CST-0242, preuve de préfixe inchangée

8 octobre 2026, Codex. Source livrée `30a69104a697fa2ea2dadd8499bde6c7cb8d72c0`,
avant `3e587b17ddc6fcc2f258276e76de135d0764a46e`, base de livraison
`bdfca8fb198e6711626c6506b816a2a656315c83`. Lecture seulement ; aucun nouveau test natif.

**L'intégration n'apporte pas le pont mémoire proposé.** Entre bdf et l'avant, aucun fichier
`src/` ne change. L'application en copie du patch de livraison `dd6e8273…` reproduit les
17 postimages du commit. Les quatre sources portant le [fragment de préfixe](../a6_prefixe_relaxed/README.md)
sont identiques à leurs captures ; les conditions et limites de ce fragment restent donc
applicables. [capture.json](capture.json), [résultats](results.json).

`forest_kernel.cpp:57–59,241` garde le chargement des feuilles et leur réécriture par l'aide
en `memory_order_relaxed`. Les parents restent également relâchés. Le noyau peut conserver
son travail sur plusieurs tranches (`pipeline_run.cpp:303–344`) ; aucune acquisition d'une
fin d'aide n'est imposée avant chaque consommation. L'attente de `hint_active` arrive à la
clôture, avant la libération des tampons : c'est une garantie de durée de vie, distincte
de l'appartenance de l'indice au préfixe consommé.

Les reprises du noyau restent ordonnées par HB : son fil sortant publie l'état disponible
avec release (`pipeline_run.cpp:346`), puis la réservation CAS acq_rel du suivant acquiert
cet état (`:182`). C'est cette chaîne, et non la seule exclusion temporelle, qui permet de
parler d'un écrivain logique ordonné. Elle ne crée pas à elle seule le transfert aide→feuille
consommée manquant dans le fragment.

La conception externe `CONCEPTION_A6.md`, SHA-256 `a2a348b2…`, conserve sa preuve par
« instant » et entrelacement, puis justifie des accès relâchés. Le README livré du microbanc
dit encore que la feuille provient d'un état antérieur. Ces arguments sont valables si
l'antériorité du préfixe est établie ; ils ne l'établissent pas pour les deux transferts
relâchés. Le fragment est toujours une réduction vérifiée sur certaines relations C++20,
pas une trame géométrique déclenchante ni une panne native démontrée.

## Recommandation au développeur

Appliquer le [pont déjà proposé](../a6_indices_concurrence/proposition.patch) : store release
de l'indice et load acquire de la feuille. Il s'applique exactement au livré, sans autre
changement ; sa postimage est épinglée dans les résultats. Garder les parents relâchés,
les publications initiales, le contrôle des cibles cellules et la garde SC de fermeture.
La [preuve](../a6_prefixe_relaxed/README.md) donne alors, pour chaque parent consulté par
l'aide, `lecture_parent HB consommation_feuille HB écriture_future` ; la cohérence interdit
de lire cette écriture future. Mettre les commentaires et la conception en accord avec
cette précondition, puis requalifier la source corrigée et mesurer son coût. Aucun coût
machine nul ni nouveau temps n'est promis ici.

La porte `pipeline_levers.cpp:130–145` appelle `hint_leaves` puis `advance_kernel` dans un
même fil. Ses 540 cas annoncés contrôlent des instantanés d'âges différents ; ils ne
quantifient pas tous les comportements mémoire concurrents. Les identités FULL déjà
[relues](../a6_identites_complementaires/README.md) restent des observations valables.
Ni elles, ni une suite native verte, ni un TSan éventuel ne remplacent ce pont de preuve.
CST-0242 reste ouvert dans cette portée.

## Périmètre du pilote livré

`microbancs/mes_t2d_a6/pilote_t2d_a6.py` construit avant depuis l'archive bdf et après depuis
le paquet ; l'A/A utilise le même binaire avant. Profil21, cache8G dans les deux bras,
Session recouverte GPU/W48 : il mesure le **lot N/I/H**, avec identités FUL1, 21 grandes
trames et ng00–02 selon la règle publiée. Il ne fournit pas une preuve C++20 ni un effet
isolé du levier I. La voie séquentielle ne lance pas les aides ; elle reste une référence
fonctionnelle, pas un examen de leur concurrence. Aucun pilote ni seuil n'est modifié ici.

```sh
python -B check.py /chemin/depot /chemin/a6_integration_20261008 --check
python -O -B check.py /chemin/depot /chemin/a6_integration_20261008 --check
```

Le lecteur contrôle les blobs Git et deux fichiers de livraison sauvegardés hors Git,
applique les patches en copie temporaire et compare leurs postimages. Normal/−O identiques.
Aucune nouvelle campagne, compilation, donnée de scène ou écriture produit.
