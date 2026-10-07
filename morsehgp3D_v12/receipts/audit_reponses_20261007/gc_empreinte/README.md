# G-c patch 1 : empreinte de résolution et travail séparés

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Lecture et petites preuves Python ; aucun moteur natif, CUDA, GCP ou jeu privé.

**Conclusion.** Le patch actif conserve le format de `res.bin` et la comparaison du travail entre nombres de fils.
Il retire bien le travail de `resolution_sha256`. C'est une empreinte de **résolution à politique fixée**, avec son
catalogue et sa numérotation fixés ; pas une empreinte FULL indépendante des politiques. Le témoin exact ci-dessous
rend cette limite concrète sans invalider le patch.

## Sources et portée

Base publiée observée avant et après preuve : `6f362f0bf4806d410657da8b3cdf36836837eb4f`.
Le dépôt local actif `v12_tour_Gc/git2` a stabilisé le patch en
`c0023455d3785ef32209bd0f91b35e3bcdd0e246`, parent `10aabcbd010bc5031ef75f6133c15b2efaa8e561`.
Les 17 sources lues au parent sont identiques à l'archive de la base publiée ; empreintes dans
[capture.json](capture.json). [active.patch](active.patch) porte ces cinq changements, sans le premier patch abandonné.
`build_p1b/CMakeCache.txt` pointe bien vers `git2/morsehgp3D_v12` ; ce build n'a pas été exécuté par l'auditeur.

## Conservation vérifiée

- `write_resolution` garde les paramètres par défaut : les huit colonnes `BKEY` à `TARG`, puis `CNTR` intégral
  (44 entiers, dont cinq compteurs objet). Aucun changement de chemin de calcul ou de mise en octets à ce défaut,
  vérifié en reconstruisant le code parent après retrait des seuls changements explicitement attendus.
  Ce contrôle de source ne remplace pas le rejeu binaire d'exports du développeur.
- `resolution_digest` sélectionne seul `object_only=true` : mêmes huit colonnes, puis `COBJ` avec les cinq premiers
  compteurs. Les noms, tailles et nombres d'éléments des sections restent hachés. `TMSK` et `TARG` sont conservés.
- Les fonctions d'admission `natural`, `decimal`, `run` et `invariants` ont exactement le même AST que le juge
  publié `650c63a1…`. Les ordres couverts et les ensembles fermés de compteurs sont donc conservés. Le corps modifié
  compare séparément l'objet puis **tous** les champs du travail, histogramme compris. Une différence dans le suffixe
  du SHA-256 est encore détectée : la comparaison porte bien sur les 64 caractères, pas le préfixe affiché.
- Onze doubles JSON donnent les codes attendus en Python normal et `-O`, résultats identiques : contrôle accepté,
  quatre refus officiels, différences objet/travail à huit fils détectées, suffixe du digest différent détecté,
  travail négatif/booléen refusé. Le résidu déjà signalé par l'auditeur principal persiste : `exit.order=false` est
  égal à `0` lors de la comparaison de dictionnaires et reste admis. Il préexiste dans le parent ; ce patch ne le ferme pas.

Les anciens SHA-256 (sections `CNTR`) et les nouveaux (`COBJ`) ne se comparent pas directement : il faut recalculer
les sections selon la nouvelle convention. Si les exports `res.bin` avant/après sont identiques à l'octet, comme le
rapporte le développeur à 22:43–22:45, cela établit l'identité de ces sorties malgré le changement de SHA. Les
34/34 portes et cette comparaison d'exports restent les preuves du développeur, sans rejeu natif dans cet audit.

## Pourquoi TARG impose une politique fixée

Témoin public `generic_11_11`, huit points, K=4. Pour la trace interne `(0,4,5,7)` de la cellule 45 à l'ordre 4,
la référence exacte donne :

| Politique | Cible | Rayon carré de la cible |
| --- | --- | --- |
| `v12_indices` | naissance 21 | `383323/4` |
| `v12_proches` | naissance 29 | `14062618234256925/109542397016` |

La trace est bien une trace stricte de cette cellule. Les sorties canoniques `nodes`, `lower`, `core`, `cover` et
`cuts` de l'ordre sont **identiques**. Le théorème D autorise plusieurs pas descendants conservant la composante,
sans imposer une même cible intermédiaire. Les positions et fractions exactes sont dans [normal.json](normal.json).
`TMSK` décrit les traces initiales canoniques, pas les parties successives d'une descente : le conserver est cohérent.

Conséquence pour G-L7 : changer seulement la visite d'un census peut changer ses compteurs et garder la même cible.
En revanche, prendre ses premiers témoins dans un nouvel ordre peut changer la règle de saut et donc `TARG`.
Annoncer alors la politique et juger la forêt/FULL canonique ; ne pas demander une égalité de ce hash de résolution
entre politiques, ni retirer `TARG` pour masquer une régression sous politique inchangée.

## Rejouer

Depuis une copie de la base publiée, appliquer `active.patch` à la racine du dépôt, puis :

```sh
python3 -B check.py /chemin/copie/morsehgp3D_v12
python3 -B -O check.py /chemin/copie/morsehgp3D_v12
```

Le script vérifie les hashes, reconstruit le parent dans un dossier temporaire, joue le juge avec ses doubles JSON
en mémoire, vérifie le filtre des colonnes par un modèle binaire indépendant, puis juge uniquement le nuage de huit sites.
Il n'exécute aucun binaire moteur et ne modifie aucun prototype. Les nouveaux préfixes CTest issus des mesures du
développeur ne sont pas requalifiés par ce reçu ; aucun gain de temps n'en découle.
