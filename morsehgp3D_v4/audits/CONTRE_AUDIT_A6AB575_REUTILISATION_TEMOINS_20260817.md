# Contre-audit ciblé : réutiliser les témoins q3 déjà certifiés

Date : 17 août 2026.  
Pin de code : `ebc82368bab03f93c2b8a480f810a93e3a8aeb74`.  
Notes rapprochées :

- `AUDIT_CONSTRUCTIF_APRES_A047460_COVER_Q3_20260817.md`, commit `a6ab575` ;
- `AUDIT_Q3_RACCORDS_COVER_ORACLE_2D26E7A_A_EBC8236_20260817.md`, commit
  `ce64844`.

Les deux audits sont compatibles. Le premier ajoute une idée immédiatement
rentable que je reçois : transporter dans le census les quelques identités de
témoins déjà certifiées par `h_cœur/h_a/h_b`. Le second détaille le passage
ultérieur au LBVH de centres et ajoute l'audit de l'oracle rationnel. Le bon
ordre combine les deux, sans transformer trois recommandations compatibles en
schisme méthodologique, activité pour laquelle le dépôt dispose déjà d'assez
de fichiers.

---

## 1. Théorème de réutilisation des témoins

Pour une ancre survivante `(a,b)`, soit

```text
base(a,b)=h_cœur+h_a(a)+h_b(b)<h_3.
```

Chaque identité comptée par ces trois certificats appartient strictement à
`W_3(a,b)` :

- le cœur est hors `A union B` ;
- `h_a` est dans `A sans {a}` ;
- `h_b` est dans `B sans {b}`.

Les trois paquets sont disjoints par construction.

Par définition du fuseau, un point de `W_3(a,b)` est strictement intérieur à
toute circum-boule d'un support q3 admissible possédé par l'ancre `(a,b)`.
Ainsi, pour chaque porteur `x`, le census peut commencer à

```text
depth=base(a,b)
```

et ignorer ces mêmes identités dans le scan du cover. Il n'y a :

- aucune fausse mort, puisque chaque crédit est strictement intérieur ;
- aucun shell caché, puisque les comparaisons des certificats sont strictes ;
- aucun double compte, si les trois listes restent disjointes et sont exclues
  du scan ;
- aucune dépendance au porteur, puisque le témoignage est universel pour
  l'ancre.

L'idée est donc mathématiquement reçue.

---

## 2. Nuance importante : `base` est un minorant certifié

Les routines actuelles ne cherchent pas nécessairement **tous** les témoins
universels du fuseau ; elles accumulent un sous-ensemble certifié suffisant
pour décider la mort. `base` est donc un minorant de la profondeur, pas la
profondeur exacte de la circum-boule.

Cela ne change pas l'optimisation : initialiser le census avec ce minorant et
scanner tous les autres sites rend exactement la profondeur finale. En
revanche, les portes doivent comparer :

- la profondeur complète pour les boules peu profondes ;
- la profondeur écrêtée à `h_3` pour les boules mortes ;
- les `InteriorIds` exacts seulement pour les événements survivants.

La formulation « mêmes profondeurs exactes » n'est pas adaptée aux boules où
les deux chemins s'arrêtent volontairement à `h_3`.

---

## 3. Comment obtenir les identités sans nouveau mur

Puisque l'ancre survit, `base<=h_3-1<=8`. Il suffit de publier au plus huit
identités.

### 3.1 Cœur

`FusedCounts` ne rend aujourd'hui qu'un nombre. Pour un rectangle vivant,
relancer une collecte bornée sur les sous-arbres effectivement crédités :

- une plage `ALL` entièrement disjointe de `A union B` peut être expansée ;
- une plage créditée après soustraction partielle doit être expansée avec
  exclusions explicites ;
- arrêter dès que le nombre déjà rendu par le compteur est atteint.

Le coût est borné par huit identités, pas par la taille du sous-arbre.

### 3.2 `h_a/h_b`

Dans la version directe actuelle, chaque succès
`universal_over_corners(...)` connaît déjà l'identité `z`. Pour chaque
extrémité, conserver une petite liste saturée à `need` en parallèle du compte.
Lors du futur passage dual-tree, garder des handles de plages `ALL`, puis les
expanser seulement pour les ancres effectivement survivantes.

### 3.3 Représentation

```cpp
struct Q3WitnessPacket {
  u8 count;
  PointId ids[8];
};
```

Les paquets peuvent être triés une fois. Le scan du cover exclut une identité
par recherche dans huit éléments, ou mieux par un petit marquage à epoch au
niveau de l'ancre.

Un porteur aigu ne peut pas appartenir au paquet : il est extérieur à la boule
diamétrale, tandis que `W_3(a,b)` est dans le fuseau intérieur. Les trois
sommets du support restent néanmoins exclus explicitement, contrat plus
lisible que de compter sur cette observation.

---

## 4. Ordre d'implémentation harmonisé

### Étape A — paquets de témoins

1. Collecter `core_ids`, `ha_ids(a)`, `hb_ids(b)` sous les mêmes autorités que
   les comptes.
2. Vérifier leur disjonction.
3. Initialiser le census à `base` et exclure ces IDs.
4. Pour un événement survivant, fusionner paquet et intérieurs nouvellement
   trouvés : cela produit déjà `InteriorIds`.

C'est la première étape, car elle réutilise du calcul déjà payé et rapproche
le probe de l'événement complet.

### Étape B — transposition site-major

À ordre de cover identique, remplacer

```text
pour chaque porteur : parcourir les sites jusqu'à saturation
```

par

```text
pour chaque site : mettre à jour les porteurs encore actifs
```

évalue exactement les mêmes couples `(porteur,site)` : chaque porteur voit le
même préfixe du cover jusqu'à atteindre `h_3`. Le gain attendu vient du layout
SoA, du chargement unique du site et des masques actifs, pas d'une diminution
magique du nombre de prédicats.

Cette voie est la bonne baseline CPU SIMD et GPU.

### Étape C — seaux radiaux

Le tri total du cover n'est pas normatif. Des seaux stables par `dist2q`
retirent `std::sort` tout en conservant une priorité approximative aux points
centraux. Toute modification d'ordre doit être jugée par digest d'événements,
car le nombre de tests peut changer sans que la sortie change.

### Étape D — LBVH des centres, cover en streaming

Pour les ancres chargées :

1. premier passage spatial : collecter seulement les porteurs possédés ;
2. construire le LBVH de leurs centres dirigés ;
3. second passage : streamer les sites ou blocs du cover vers ce LBVH, sans
   matérialiser et trier des centaines de `CoverPoint` ;
4. utiliser les `range-add` de la forme affine `ell_z(T)` ;
5. conserver le scan site-major plat pour les petites ancres.

Les paquets de témoins initialisent les compteurs du LBVH exactement comme ils
initialisent la baseline plate.

---

## 5. Portes causales

### Paquets

1. `packet=off|on` : mêmes digests de `SupportKey`.
2. Pour les événements survivants : mêmes `InteriorIds` triés contre l'oracle
   rationnel de `ebc8236`.
3. Aucun ID commun entre `core`, `ha`, `hb`.
4. Aucun ID du paquet recompte dans le scan.
5. Mutant « shell autorisé dans le paquet » tué.
6. Mutant « plage partiellement superposée à A/B réutilisée entière » tué.

### Site-major

7. Même digest que carrier-major.
8. Même profondeur écrêtée par porteur.
9. Même liste de shells pour les boules survivantes.
10. `power_tests` identique à ordre strictement identique ; si des seaux
    changent l'ordre, exiger seulement la même sortie et publier les deux
    nombres.

### LBVH

11. Parité avec le scan plat sur chaque ancre de petits nuages.
12. Porte d'activité : au moins un `range-add`, au moins un prune et au moins
    une subdivision.
13. Fixture corrélée `ell_x(T_x)=0` : le porteur ne doit jamais être crédité
    comme intérieur de sa propre boule.

---

## 6. Ajustement de priorité après `ebc8236`

L'oracle indépendant demandé par l'autre audit existe maintenant. Il doit
servir immédiatement à juger les paquets et les `InteriorIds`, après ajout du
self-test OBig et remplacement de `abort` par un statut contrôlé.

Ordre unique conseillé :

1. corriger IDs/owner, digests et portes `tree|cover|exact` ;
2. self-test OBig et statut de débordement ;
3. paquets de témoins et `InteriorIds` ;
4. scan site-major et seaux ;
5. événement q3 complet (`BallKey`, niveau, facettes, multifusion) ;
6. LBVH de centres avec streaming du cover ;
7. q4 seulement après parité de cette chaîne.

---

## Conclusion

La proposition de réutilisation des témoins complète très bien le cover
partagé. Le préfiltre ne doit plus jeter les certificats après avoir rendu un
booléen : dans le régime vivant, il ne possède qu'une poignée d'identités, qui
sont précisément les premières identités intérieures de tous les événements de
l'ancre.

La chaîne q3 peut donc gagner simultanément en vitesse et en complétude de
payload : paquets bornés, scan site-major, puis LBVH. C'est une progression
plus sûre que de sauter directement vers un arrangement ou q4, ces deux
activités ayant déjà démontré ailleurs leur aptitude à transformer une bonne
idée en plusieurs semaines de déterminants.