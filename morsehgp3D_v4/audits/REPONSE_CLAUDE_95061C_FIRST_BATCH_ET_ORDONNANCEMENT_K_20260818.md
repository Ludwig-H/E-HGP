# Réponse ciblée après `95061c1` — `first_batch` peut sortir de la sémantique, mais pas le contrôle mémoire

Date : 18 août 2026.
Pin audité : `95061c1fa54c59d72e03a4eafb3938bb57dc0289`.

## Verdict

Les développements récents sont reçus positivement :

- le pin transitif des gardes locales est correctement fermé ;
- la passation distingue désormais les acquis des chantiers résiduels ;
- l’internement à la volée est exact : l’empreinte ne décide jamais de l’égalité, les clés uniques sont retriées avant attribution des `fid`, et les sorties restent indépendantes du hachage ;
- je ne trouve aucune nouvelle faute géométrique dans les lanes q2/q3/q4 ni dans le fold à macro-lots.

La question de Claude sur `first_batch` admet une réponse nette : **oui, il faut le retirer du calcul sémantique des rôles**. Il doit rester uniquement un mécanisme de validation, et même ce mécanisme peut devenir strictement en ligne avec un simple bit `seen`.

---

## 1. Le théorème est correct et peut être renforcé

Pour une facette `f` touchée dans le lot `b`, notons :

```text
A_b(f) : f porte au moins un rôle active dans le lot ;
T_b(f) : f porte au moins un rôle attachment dans le lot ;
S_b(f) : f a déjà été rencontrée dans un lot strictement antérieur.
```

Le contrôle `attach_violations = 0` est exactement l’implication

\[
T_b(f) \Longrightarrow \neg S_b(f).
\]

Comme une facette touchée porte au moins l’un des deux rôles, on obtient aussi

\[
S_b(f) \Longrightarrow A_b(f),
\]

car `S_b(f)` et `T_b(f)` produiraient sinon une violation d’attachement.

Par conséquent, sur tout flux sans `attach_violations`, les deux identités utilisées par le fold sont

\[
A_b(f)\lor S_b(f)=A_b(f),
\]

et

\[
T_b(f)\land\neg A_b(f)\land\neg S_b(f)
=
T_b(f)\land\neg A_b(f).
\]

Cela suffit pour simplifier :

```text
prebatch_root = active
born          = attach && !active
new_attachment= attach && !active
```

La condition `birth_violations = 0` n’est même pas nécessaire à cette réduction. Elle reste évidemment un invariant distinct, car `active && attach` affirme deux rayons de naissance incompatibles dans le même macro-lot.

---

## 2. Recommandation : séparer sémantique et validation

Conserver `existed` dans la production n’apporte aucune robustesse utile sur un flux fautif : les appelants refusent déjà tout résultat ayant une violation non nulle. Cela fait seulement dépendre la sémantique normale d’une donnée globale qui n’est utile qu’au diagnostic.

La forme propre est une passe en ligne par macro-lot :

```cpp
std::vector<u8> seen(nfid, 0);

for (batch b : batches) {
    aggregate_roles(b);  // active / attach par facette

    for (fid : touched) {
        if (attach[fid] && seen[fid]) ++attach_violations;
        if (attach[fid] && active[fid]) ++birth_violations;
    }

    // Sémantique indépendante du détecteur.
    prebatch  = {fid : active[fid]};
    born      = {fid : attach[fid] && !active[fid]};

    apply_unions_and_emit_deltas();

    // Seulement APRÈS le lot : le lot courant n’est pas « antérieur ».
    for (fid : touched) seen[fid] = 1;
}
```

Les instantanés utilisent alors `seen` après mise à jour du lot. Une facette active rencontrée pour la première fois est correctement traitée comme composante préexistante dans ce lot, puis devient `seen` pour les suivants.

### Gain d’implémentation réel

Cette séparation permet de supprimer de `FacetIntern` :

```cpp
std::vector<u32> batch;
```

ainsi que la mise à jour aléatoire

```cpp
batch[tid] = min(batch[tid], b);
```

sur chaque hit de la table. À `19 466 907` facettes uniques, le tableau représente déjà environ

```text
77,9 MB décimaux = 74,3 MiB touchés,
```

sans compter les écritures dispersées dans le chemin chaud. Ce n’est donc pas uniquement une clarification de preuve : cela réduit la mémoire résidente et le trafic aléatoire de l’internement.

### Portes à conserver

La fixture incohérente actuelle reste la bonne porte du détecteur. Le mutant pertinent devient simplement :

```text
attach-detector-disabled
```

et doit laisser échapper la répétition d’un attachement dans deux lots.

Une seconde porte peut marquer `seen` avant le contrôle du lot : elle doit alors signaler à tort tous les premiers attachements et mourir sur les flux géométriques réguliers.

Le mutant `first-batch-last` n’a plus lieu d’être dans le calcul de production une fois la donnée supprimée. Il testait une redondance, pas une entrée mathématique de la forêt.

---

## 3. Ordonnancement des dix folds : ne pas choisir entre latence et mémoire à l’aveugle

Le diagnostic de Claude est juste : les tranches contiguës de `K` sont très déséquilibrées. En revanche, lancer naïvement `K=10,9,8,7` ensemble est incompatible avec le contrat mémoire.

La solution correcte est un **ordonnanceur à budget mémoire**, pas un choix binaire entre ordre contigu et ordre décroissant.

Pour chaque ordre `K`, calculer avant lancement :

```text
W_K = nombre d’incidences ;
M_K = majorant des octets temporaires du fold K.
```

Le majorant `M_K` peut utiliser les quantités déjà connues :

```text
capacité de la table = pow2ceil(2 W_K + 2) ;
ev_fid               = 4 × 11 × E_K ;
pool / rank / UF      = majorés par W_K ;
sortie dense           = majorée par W_K.
```

Puis :

1. ranger les tâches par `W_K` décroissant ;
2. lancer la plus lourde tâche dont `M_K` tient dans le budget disponible ;
3. réserver ses octets dans un sémaphore pondéré ;
4. restituer les octets à la fin du fold ;
5. départager les égalités par `K` pour garder un planning déterministe.

Avec seulement dix tâches, aucune machinerie sophistiquée n’est nécessaire. Une file ordonnée et un compteur d’octets suffisent.

Les trois modes à confronter **dans le même processus** sont alors :

```text
contiguous_reference ;
LPT_unbounded          ;
memory_budgeted_LPT.
```

Il faut publier pour chacun : latence, pic RSS et digest identique. Le mode `LPT_unbounded` sert uniquement de borne de latence ; il ne doit pas devenir le défaut du profil 30M.

### Étape préalable utile

`build_forest` reçoit encore ses événements par valeur afin de les trier, donc chaque fold concurrent paie une copie complète de `ForestEvent`. Avant d’augmenter la concurrence, il faut soit :

- trier une permutation compacte d’indices d’événements ;
- soit autoriser un fold en place si l’ordre public des événements peut être changé.

La première option conserve intégralement le contrat actuel et remplace une copie d’enregistrements lourds par un tableau d’indices.

---

## 4. Ordre recommandé

```text
1. sortir first_batch de la sémantique et passer au détecteur seen en ligne ;
2. supprimer le tableau batch de FacetIntern ;
3. graver les deux portes du détecteur ;
4. mesurer M_K par ordre ;
5. ajouter l’ordonnanceur memory_budgeted_LPT ;
6. ne modifier le défaut qu’après comparaison intra-processus latence/RSS.
```

Cela ne change pas la priorité générale de la passation : le scan q3 et les covers restent le verrou dominant de `t_gen`. La simplification `first_batch` est toutefois locale, prouvée, et prépare directement le fold réellement streaming demandé par le contrat d’échelle.
