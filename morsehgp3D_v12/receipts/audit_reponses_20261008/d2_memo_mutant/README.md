# CST-0104 : proposition de mutant de date

[Patch du manifeste](proposition.patch), adapté au produit livré
`d2f39fe82` : ajoute `date_initiale_remplacee_par_date_terminale` aux sept
mutants `tower`, porte **`mhgp12_tower_unit_witness_memo`**, plancher 8.
Aucun fichier produit ou manifeste vivant modifié. **Ni compilation ni
exécution native** : cette proposition ne clôt pas la preuve causale restante.
Les [pins](pins.json) et le [résultat](results.json) permettent la relecture.

Le mutant compare la date de **la seule cible terminale** au rang de la
jonction appelante, tout en conservant les contrôles de décroissance interne :

1. À la première plus petite boule (`chain == 1`), il adopte son rang ou
   son niveau exact comme `Previous`, sans comparaison à `junction_rank`.
   Les deux routes sont changées : boule déjà au catalogue, ou certificat
   exact suivi du census. L’incrément du compteur `controls` est conservé.
2. Les boules suivantes restent strictement descendantes via les fonctions
   `control_rank`/`control_level` **inchangées**. Une première sonde qui
   trouve directement une naissance reste contrôlée normalement.
3. Avant les retours naissance et cellule, il exige que le rang terminal
   soit strictement inférieur à la jonction. Ce n’est donc pas une simple
   suppression de tous les contrôles de date.

Le témoin réel atteint précisément ces modifications. Sur la ligne
`{0,2,4,6}`, `witness_memo` demande de résoudre `{0,6}` à l’ordre 2 sous
la jonction de niveau 4. Sa première boule a niveau **9** ; les deux sites
strictement intérieurs sont `{2,4}`, dont la naissance a niveau **1**.

| Construction du témoin | Première route | Code correct | Mutant attendu |
|---|---|---|---|
| Kmax 2 | boule 9 hors Cat₂ ; certificat, census saturé | refuse 9 ≥ 4 | accepte la cible 1 < 4 |
| Kmax 3 | boule 9 présente dans Cat₃ ; saut depuis p=2 | refuse 9 ≥ 4 | accepte la cible 1 < 4 |

Le témoin exige `Reason::tower_invariant` et `order=2` après cet appel :
un succès doit faire échouer ces vérifications, **par code**, sans signal,
exception, délai ou mémoire invalide. Les constructions légitimes exécutées
avant cet appel restent dans le domaine `β(F₀) < jonction` ; le mutant n’en
change ni la cible ni les compteurs. Cette dernière affirmation est une
lecture du flot, pas un succès natif constaté.

Le contrôle Python applique le patch par `git apply --check`, puis utilise
les vraies fonctions `load_manifest`, `check_patterns` et `mutated_files`
du lanceur épinglé sur une copie temporaire. Les huit mutants s’appliquent ;
les quatre remplacements du nouveau mutant ne touchent que `resolve_part`.
Les fonctions de contrôle partagées restent identiques. Les calculs de date
Fraction distinguent les deux refus MEMO devenant succès, le cas D2 valide,
une cible terminale trop tardive et une étape non descendante. Ils ne se
substituent pas à la compilation de la mutation.

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/d2_memo_mutant/check.py --check
python3 -O -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/d2_memo_mutant/check.py --check
```

Lectures normal/−O identiques. Le développeur doit ensuite exécuter la porte
nominale et la mutation via son lanceur, conserver le verdict par code et
l’arbre qualifié. Si le manifeste Gc à 18 mutants est livré entretemps,
ajouter uniquement cette entrée à la nouvelle cohorte et vérifier les
motifs ; ne pas écraser son plancher avec 8 ni transférer notre pin.
