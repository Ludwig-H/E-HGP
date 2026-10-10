# Q3 — Première sonde sans matérialiser immédiatement Part

10 octobre 2026 ; source `aa6338ee8`, quatre fichiers épinglés dans
`capture.json`. Réponse à [Q3 du développeur](../../developpement_20261010/QUESTION_CLAUDE_g_travail.md).
`phase=exploration_v12_hors_registre`, `objet=full_pi0`, `public_status=not_claimed`.
**Proposition mathématique et modèle Python seulement ; aucun moteur modifié,
aucune mesure de gain.**

La cible peut être cherchée sans écrire immédiatement `Part`, à condition de
conserver **le masque du représentant**. Une vue immuable `(I,U,mask)` énumère
les sites de `I ∪ {U[j] : bit j de mask}` en ordre croissant. Préconditions :
I et U strictement triés, sans doublon et disjoints ; `|U| ≤ 64`, aucun bit hors
de U, `|I| + popcount(mask) = k ≤ 12`. Ce sont des invariants à conserver à leurs
frontières de certification, pas une recommandation de rescanner I/U à chaque
sonde. En C++, le contrôle des bits excédentaires doit traiter `|U| = 64` sans
décalage de 64 bits.

Preuve : à chaque pas, la fusion virtuelle émet le minimum des deux prochains
sites non consommés. Par induction, elle produit exactement les k mots que
`build_trace` matérialise aujourd'hui. Un comparateur recommençant cette fusion
**à chaque comparaison** a donc le même signe lexicographique que `compare_rows`.
L'empreinte additive est identique, donc le seau, chaque comparaison de la
dichotomie, son résultat et sa vérification exacte sont identiques, même avec
**toutes les empreintes forcées à zéro**. L'empreinte seule n'est jamais un
certificat d'égalité. La borne reste O(k log E), sans nouveau théorème de coût
meilleur : refusionner pendant les comparaisons peut coûter plus cher.

`passes.cpp` calcule **déjà** l'empreinte de I et les empreintes individuelles de U
une fois par cellule, puis la somme des bits sélectionnés. Ce calcul n'est donc
pas un gain à proposer. Garder la file produit de **16 entrées**, le préchargement
du répertoire à l'entrée, celui du seau huit entrées plus tard et le même ordre
de sortie. Les vues doivent posséder leurs spans/masques par valeur et emprunter
le catalogue immuable, dont la durée de vie couvre la file ; aucune référence
aux variables locales d'une cellule ni au tableau `shell_keys` réutilisé.

Sur hit, préserver l'admission de `resolve_part` (table présente, rang de jonction
valide), puis `probes += 1`, `controls += 1` et **rang de naissance strictement
inférieur à celui de la jonction avant toute sortie**. Rang égal ou supérieur :
refus, sans incrément de `first_probe_hits` ni d'histogramme. Sur succès seulement,
conserver `first_probe_hits`, histogramme de chaîne zéro, maximum et événement
de profilage. Sur miss, former `Part` une fois et appeler le même resolveur avec
`FirstProbe{done=true, hit=nullopt}` : aucune seconde première sonde, aucun double
compte. Factoriser le préfixe commun serait préférable à en dupliquer les règles.
Le reste de la descente ne change pas et n'est pas simulé ici.

**I/U sans le masque ne donnent pas une cible unique.** Pour les trois sites
entiers `(0,0,0)`, `(4,0,0)`, `(2,3,0)`, la boule triple a centre `(2,5/6,0)` et
niveau `169/36`. Ses barycentriques `(13/36,13/36,5/18)` sont strictement positifs,
son intérieur est vide et sa coquille contient les trois sites. À k=2, les trois
masques de deux sites mènent aux **trois naissances distinctes** des boules de
diamètre correspondant, de niveaux `4`, `13/4`, `13/4`, tous strictement
inférieurs à `169/36`. Pour chacune, le troisième site est strictement extérieur.
L'égalité de niveau des deux dernières ne fusionne pas leurs identités.

Le modèle indépendant compare l'oracle `sorted(I + sélection(U))` à l'itérateur :
**3 584 vues, 14 336 recherches, 50 880 comparaisons**, quatre masques de hash dont
zéro ; 7 424 hits et 6 912 misses. Cas dirigés à k=12 et au bit 63, table vide,
mauvais ordre, miss sous collision ; **dix préconditions invalides refusées**,
dont masque hors coquille avec popcount correct, cardinalité incorrecte, I/U
non triés, doublons et chevauchement. Seize cas de rang/admission/compteurs et
36 files de longueurs 0,1,7,8,15,16,17,33,65 conservent les résultats et l'ordre
des événements. Le nombre de matérialisations évitées est un compte du modèle,
pas une mesure du produit.

```sh
python3 -B check.py /chemin/du/depot
python3 -B -O check.py /chemin/du/depot
```

Avant adoption : porte native équivalente contre `Part`, identités FULL et
compteurs, puis microbanc déclaré comparant coût de trace, recherches et G/FULL.
Le profil des premières sondes cité par le développeur vient de B2 : il ne suffit
pas à prédire le gain sur A6c/B3. Une réduction du temps « trace » peut simplement
déplacer du travail vers les comparaisons de la table.
