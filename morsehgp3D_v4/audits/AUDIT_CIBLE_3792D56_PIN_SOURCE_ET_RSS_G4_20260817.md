# Audit ciblé après `63d364a` et `3792d56` — pinner la source réellement mesurée et fiabiliser le pic RSS

Date : 17 août 2026.  
Pins audités :

- axial borné : `63d364a46de1bbd5f02a98e259492021bf0c538b` ;
- campagne transactionnelle : `3792d56db71399fcb6ac70cf1562589ae6a40107`.

## Verdict

Les deux avancées sont reçues positivement.

### Axial borné

La sélection axiale est mathématiquement cohérente :

- les permanents `B=0, A<0` sont comptés strictement ;
- le côté `B>0` conserve les plus petites racines, le côté `B<0` les plus grandes ;
- la normalisation `(-A,-B)` conserve bien `mu=A/B` ;
- le seuil est calculé avec multiplicité et le groupe d'égalité frontière est conservé entier ;
- un groupe exact de `mu` produit une seule sphère ;
- le représentant retenu est le minimum de `ball_candidate_less` parmi **toutes** les complétions valides du groupe ;
- le scan de profondeur reste exact sur cette sphère commune.

La porte appariée a d'ailleurs attrapé la première erreur de direction sur le côté négatif. Le verdict CPU négatif est correctement assumé : garder ce chemin en opt-in, comme candidat GPU régulier, est la bonne décision. Je ne vois pas de verrou de correction restant dans `63d364a`.

### Campagne transactionnelle

`3792d56` ferme bien les défauts précédemment signalés : codes de runs toujours matérialisés, statuts atomiques, séparation latence isolée / débit contendu, concurrence bornée par la mémoire, validation locale après rapatriement, et format machine `juge=off desaccords=NA`.

Il reste toutefois **deux raccords de protocole réellement importants avant la session payante**.

---

## 1. Le tar mesuré n'est pas encore relié à un commit propre

Le script empaquette l'état courant du répertoire :

```bash
tar czf "${TAR}" --exclude=build --exclude=.git morsehgp3D_v4
```

mais il ne vérifie pas que cet état correspond à `HEAD`. Des modifications locales non commitées, voire des fichiers non suivis sous `morsehgp3D_v4`, peuvent donc entrer dans la campagne.

La tentative de journalisation distante :

```bash
git -C '"${REPO_ROOT}"' rev-parse HEAD 2>/dev/null || true
```

ne ferme pas ce contrat :

- `${REPO_ROOT}` est un chemin **local**, injecté dans la commande distante ;
- le tar transféré ne contient aucun `.git` ;
- l'erreur est volontairement supprimée par `|| true`.

Le reçu peut donc connaître le nom du dépôt et tous les compteurs, sans connaître exactement le code qui les a produits. Ce serait regrettable après avoir rendu transactionnels les vingt-huit processus eux-mêmes.

### Correction minimale

Avant de démarrer la VM :

```bash
SOURCE_COMMIT="$(git rev-parse HEAD)"

git diff --quiet -- morsehgp3D_v4
git diff --cached --quiet -- morsehgp3D_v4

if [ -n "$(git ls-files --others --exclude-standard -- morsehgp3D_v4)" ]; then
  echo "REFUS : fichiers non suivis dans morsehgp3D_v4" >&2
  exit 2
fi
```

Puis calculer un digest du payload exact :

```bash
SOURCE_TAR_SHA256="$(sha256sum "${TAR}" | awk '{print $1}')"
```

Le couple

```text
source_commit,
source_tar_sha256
```

doit être :

1. écrit dans le journal local avant le démarrage ;
2. transféré avec le tar ;
3. inclus dans chaque fichier `.status` ;
4. vérifié identique sur tous les runs par le validateur local ;
5. recopié dans le reçu final.

Le `git -C ...` distant doit être supprimé. La source distante n'est volontairement pas un clone Git ; son autorité est le digest du tar préparé depuis un arbre propre.

Une variante encore plus robuste est de créer le tar depuis Git :

```bash
git archive --format=tar.gz -o "${TAR}" "${SOURCE_COMMIT}:morsehgp3D_v4"
```

Elle exclut par construction tout état non commité. Il faut alors choisir explicitement si les reçus nouvellement générés mais non encore commités doivent faire partie du payload. Pour une campagne contractuelle, la réponse la plus saine est non.

---

## 2. Le repli `VmHWM` observe le wrapper `timeout`, pas nécessairement le probe

Lorsque `/usr/bin/time` est absent, le script fait :

```bash
timeout 10800 taskset ... "$P" ... &
pp=$!
grep VmHWM /proc/${pp}/status
```

`pp` désigne le processus `timeout`. Celui-ci lance et surveille le vrai probe. Son `VmHWM` propre peut rester minuscule pendant que l'enfant consomme plusieurs gigaoctets.

La valeur sous-estimée alimente ensuite :

```text
concurrence = floor(0.75 * RAM / peak_RSS)
```

Le mécanisme censé prévenir l'OOM peut donc précisément autoriser une concurrence excessive lorsque le repli est utilisé.

### Route recommandée

Pour cette campagne, le plus sûr est de rendre GNU `time` obligatoire et de refuser avant toute mesure s'il manque :

```bash
test -x /usr/bin/time || {
  echo "REFUS : /usr/bin/time requis pour une campagne RSS-pilotée" >&2
  exit 2
}
```

Cela supprime un repli complexe et non testé. L'image G4 doit démontrer cette précondition pendant la phase build, avant les pilotes lourds.

Si un repli doit absolument subsister, il faut mesurer le **groupe de processus** ou le cgroup du run, pas `/proc/$pp` seul. Par exemple :

- lancer chaque run dans un cgroup dédié et lire `memory.peak` ;
- ou parcourir récursivement les descendants du wrapper et sommer/majorer leurs RSS ;
- puis graver un selftest où un enfant alloue une quantité connue de mémoire, afin que le wrapper seul ne puisse passer.

Je conseille la première route : exiger `/usr/bin/time` maintenant, cgroup lors d'une future industrialisation.

---

## 3. Petite porte de campagne utile

Avant la G4, ajouter un mode local ou une fonction testable qui utilise un faux probe :

```text
run A : sortie valide, code 0 ;
run B : code 7 ;
run C : timeout ;
run D : allocation mémoire connue.
```

La validation doit produire :

```text
A -> valide,
B/C -> partial_or_failed avec codes présents,
D -> peak_rss_kb au-dessus du plancher,
```

et tous les statuts doivent porter le même `source_commit/source_tar_sha256`.

Il n'est pas nécessaire d'ajouter ce selftest à la suite mathématique de 150 secondes ; un test shell séparé suffit. Il protège les heures de calcul qui suivent, activité généralement plus rentable que de découvrir le fonctionnement de `set -e` au milieu d'une facture cloud.

## Ordre conseillé à Claude

1. pinner l'arbre Git propre et le digest du tar ;
2. inclure ces deux champs dans tous les statuts et dans le validateur ;
3. rendre `/usr/bin/time` obligatoire ou corriger réellement la mesure par groupe de processus ;
4. graver le petit selftest transactionnel ;
5. lancer ensuite la phase de latence isolée, puis les vagues de couverture.

## Conclusion

L'axial borné et la réécriture transactionnelle sont reçus. Le script sait désormais dire si ses runs ont réussi ; il doit encore savoir **quel code exact** ils ont exécuté et **quelle mémoire le vrai processus** a consommée.

Ces deux corrections sont locales. Une fois posées, je ne vois plus de raison d'empêcher la campagne G4 pendant que le fold `sort/reduce` progresse en parallèle.
