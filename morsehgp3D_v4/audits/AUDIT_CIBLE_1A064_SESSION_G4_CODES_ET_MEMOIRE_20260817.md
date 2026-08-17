# Audit ciblé de la session G4 après `1a0640` — fiabiliser la campagne avant lancement

Date : 17 août 2026.  
Pin de code mesuré : `06795db2b099d146409aca0991060cfb992260e5`.  
Pin de campagne : `1a06402332a0c0ca405ee5fe794f69f297f42924`.  
Audit de fold déjà présent et reçu : `d0edac9de6a570476f48e61bd94abd3087c9b863`.

## Verdict

Les progrès algorithmiques sont reçus positivement :

- le compte exact de `W_4(a,b)` sur le cover tue sûrement une ancre entière avant `seed × complétion` ;
- la campagne locale `n=8000` confirme que le flux publié reste proche d’un candidat par événement ;
- le prochain verrou de calcul est bien partagé entre génération q4 et fold ;
- la proposition `FacetOccurrenceTable + sort/reduce` de `d0edac9` est la bonne direction pour remplacer les `std::map` sans changer l’objet.

Je ne vois pas de nouveau verrou mathématique dans `06795db`. En revanche, le script

```text
gcp-migration/session_campagne_v4_scale_g4.sh
```

doit être corrigé avant son lancement. Deux défauts peuvent rendre la campagne ininterprétable ou la faire échouer par mémoire indépendamment du code HGP.

---

## 1. Sous `set -e`, les codes non nuls ne sont pas enregistrés

Le shell distant commence par :

```bash
set -euo pipefail
```

et chaque job exécute :

```bash
(
  timeout 10800 "$P" ... > "$file" 2>&1
  echo "code=$?" >> "$file"
  ...
) &
```

Si `timeout` rend `124`, si le noyau tue le processus par manque de mémoire, ou si le probe rend un autre code non nul, `set -e` arrête immédiatement le sous-shell. La ligne

```bash
echo "code=$?"
```

n’est jamais atteinte.

Le commentaire du script affirme donc à tort qu’un timeout sera inscrit comme `code=124`. Le `wait || true` masque ensuite les échecs et le script imprime inconditionnellement :

```text
=== CAMPAGNE COMPLETE ===
```

Une matrice partielle ou entièrement morte peut ainsi être présentée comme terminée, avec des fichiers sans code final.

### Correction minimale

Chaque job doit capturer explicitement son code dans un contexte où `errexit` ne l’interrompt pas :

```bash
run_one() {
  local fam="$1" n="$2" smax="$3"
  local file="out/scale_${fam}_n${n}_smax${smax}.txt"
  local rc=0

  /usr/bin/time -v -o "${file}.time" \
    timeout 10800 "$P" \
      --family="$fam" --n="$n" --s=8 --smax="$smax" --seed=3 \
      --min-balls=10000 --min-fusions=1000 \
      >"$file" 2>&1 || rc=$?

  printf 'code=%d\n' "$rc" >>"$file"
  printf '%s\t%s\t%s\t%s\n' "$fam" "$n" "$smax" "$rc" \
    >> out/manifest.tsv
  printf '%s\n' "--- fini ${fam} n=${n} smax=${smax} code=${rc}"
  cat "$file"
}
```

Après tous les `wait`, le script doit compter les 24 entrées du manifeste et publier l’un des statuts :

```text
complete       : 24/24 codes 0 ;
partial        : au moins un code 124 ou arrêt externe ;
failed         : code autre que 0/124, fichier absent ou manifeste incomplet.
```

Le mot `COMPLETE` ne doit être imprimé que dans le premier cas.

---

## 2. Les 24 processus sont bornés en CPU, pas en mémoire

Le script lance simultanément :

```text
4 familles × 3 tailles × 2 profils = 24 processus
```

sur une machine annoncée à 180 Go. Cette concurrence est choisie d’après les 48 vCPU, mais aucune mesure de RSS n’est utilisée.

Or le run `uniform,n=8000,smax=11` produit déjà :

```text
3 126 158 événements,
19 465 140 fusions,
1 974 086 nœuds.
```

Le chemin courant conserve les événements puis utilise plusieurs `std::map` pour les facettes, les rôles, les composantes et le rendu. Les runs `n=16000` et surtout `n=32000` auront donc une emprise de plusieurs gigaoctets chacun avant même la future table d’incidences. Huit runs `n=32000`, huit runs `n=16000` et huit runs `n=8000` lancés ensemble n’ont aucune garantie de tenir dans 180 Go.

Un OOM global serait particulièrement mauvais ici : il tuerait des jobs dans un ordre dépendant du noyau, précisément pendant une campagne destinée à comparer des pentes.

### Porte nécessaire : mesurer le RSS avant de choisir la concurrence

Le `/usr/bin/time -v` proposé ci-dessus doit enregistrer au minimum :

```text
Maximum resident set size,
elapsed time,
exit code.
```

La campagne doit être organisée par vagues :

1. un pilote `uniform` par couple `(n,smax)` ;
2. lecture du RSS maximal observé ;
3. calcul de la concurrence sûre avec une réserve explicite, par exemple

```text
jobs_mem = floor(0.75 × MemTotal / rss_pilot_majoré),
jobs = min(jobs_cpu, jobs_mem).
```

Le RSS doit être majoré, par exemple de 25 %, car `terrain`, les plateaux et le nombre de facettes peuvent dépasser `uniform`.

À défaut d’un ordonnanceur pondéré, une politique provisoire honnête est :

```text
n=8000  : concurrence configurable, après pilote ;
n=16000 : concurrence divisée au moins par deux ;
n=32000 : un ou deux processus au maximum avant mesure contraire.
```

Il faut lancer les tailles par vagues plutôt que les 24 runs ensemble. Le script doit publier la concurrence réellement employée et le RSS maximal de chaque run.

### Raccord avec le fold `sort/reduce`

L’audit `d0edac9` conseille à juste titre de traiter les ordres `K` l’un après l’autre. La même discipline doit être conservée dans la conception mémoire :

```text
peak_bytes par K,
occurrences maximales d’un K,
aucune matérialisation simultanée des dix ordres.
```

La campagne actuelle doit mesurer le RSS de la version `map`; la future version `sort` devra être comparée à la fois en temps et en pic mémoire. Une baisse de temps obtenue en multipliant le pic mémoire ne constituerait pas une route GPU viable.

---

## 3. Les résultats partiels ne sont pas encore rapatriés durablement

Les fichiers distants sont copiés par `scp` seulement après la fin de la commande SSH contenant toute la matrice. Si le shutdown invité coupe la VM avant cette étape, le journal local contient les sorties affichées par `cat`, mais `${WORK}/out` n’est pas rempli contrairement au message du `cleanup`.

Ce n’est pas bloquant pour la sûreté de la VM, mais cela affaiblit fortement la recette scientifique. Deux corrections simples :

- exécuter la campagne par vagues et faire un `scp` après chaque vague ;
- ou pousser chaque fichier terminé vers un stockage durable immédiatement après l’écriture de son code et de son fichier `.time`.

Le manifeste local doit alors permettre de distinguer sans ambiguïté : run terminé, timeout, OOM, non démarré et résultat non rapatrié.

---

## Ordre utile

1. Corriger la capture des codes sous `set -e` et le statut final de matrice.
2. Ajouter `/usr/bin/time -v`, le manifeste et une concurrence pilotée par le RSS.
3. Rapatrier les résultats après chaque vague.
4. Lancer d’abord une vague `n=8000`, puis calibrer `n=16000/32000`.
5. En parallèle, exécuter l’audit `d0edac9` sur le fold `sort/reduce` en conservant `--fold=map|sort` sur les petites portes.

## Conclusion

Le run `n=8000` est une vraie avancée : l’objet produit a une taille raisonnable et les filtres q4 sont reçus. La prochaine campagne doit maintenant être aussi exacte dans sa comptabilité des processus que le pipeline l’est dans sa comptabilité des témoins.

Les corrections demandées ne changent ni les mathématiques ni les mesures individuelles. Elles empêchent simplement un timeout ou un OOM de devenir silencieusement une ligne manquante sous une bannière `CAMPAGNE COMPLETE`.
