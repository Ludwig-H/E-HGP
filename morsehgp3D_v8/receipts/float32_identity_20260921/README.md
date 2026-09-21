# Clés float32 et événements q4 — qualification commune

21 septembre 2026. CPU local, `lossless_float32_input_only`,
`native_identity_and_events`, exploration hors registre, `not_claimed`.
Voir le [contrat numérique](../../docs/IDENTITE_FLOAT32_ET_EVENEMENTS_Q4_20260921.md).
Ni census/générateur natif, ni catalogue, ni tour FULL, ni GCP dans ce lot.

## Captures closes

| Capture | Profil | Commandes | Verdict |
|---|---|---:|---|
| `release/identity_0ecgriwi` | GCC Release | 28 | PASS |
| `sanitize/identity_4o1kodni` | Clang ASan/UBSan, fuites activées | 28 | PASS |
| `mutations` | Référence et deux mutations compilées, GCC | 43 | PASS |

Builds désormais épinglés : `build/v8_float32_identity_20260921`,
`build/v8_float32_identity_sanitize_20260921` et
`build/v8_float32_identity_mutants_20260921`. Aucun échec de qualification
dans ces trois captures ; les préflights natifs distincts ne les remplacent pas.

## Protocole

`bench/run_float32_identity_checks.py` compile huit unités dans un build
neuf : boules, clés, événements, leurs trois sondes, ancien test entier
et nouveau test PGCD/division. Cinq exécutables,28commandes par profil :
qualification GCC Release puis Clang ASan/UBSan, détection des fuites
activée explicitement. Les deux gates rationnelles tournent normalement
et sous Python−O ; leurs verdicts et mutations sont rejugés à la lecture.

Les dépendances des huit unités sont hachées **avant toute compilation**,
puis comparées aux dépendances compilées. Dix-sept sources locales,
objets, binaires, commandes, environnements et sorties brutes/base64 sont
clos et revérifiés avant/après lecture. Le lecteur exige les sources,
builds et dépendances vivants : ce n'est pas une archive autonome.

Les captures précédentes restent historiques, notamment
`float32_ball_20260921` à `12d885d8`. Leur lecteur vivant refuse à juste
titre le header entier désormais étendu. Ce lot réexécute effectivement
les anciens tests ; il ne transforme pas leur ancien verdict en preuve
du nouveau code. CMake et le moteur u16 restent inchangés ; pas de
nouvelle exécution de toute la suite CTest revendiquée ici.

## Couverture

L'oracle résout le centre et le rayon par élimination rationnelle, puis
forme et normalise les coefficients globaux. Il ne reprend pas les
déterminants du code C++. Les tests comparent les cinq coefficients et
l'encodage canonique, ainsi que le chemin d'émission depuis un support
déjà certifié. La sphère de centre0/rayon5 a des supports positifs de
toutes les arités2/3/4 ; permutations, translations, exposants extrêmes,
signes de zéro et centres/rayons voisins sont séparément exercés.

Pour les événements, les racines de l'oracle viennent directement du
centre rationnel et de la normale orientée. Égaux, signes opposés des
dénominateurs, coplanaires et seeds non aiguës sont distingués. Les
selftests couvrent quatre arrondis, FTZ/DAZ, drapeaux d'exception du
mode entier et lectures concurrentes avec états privés, sans gate TSan.

L'extension entière a9051contrôles :185divisions exactes,1124PGCD,
47séries de décalages,347refus pour reste,9diviseurs invalides et
9dépassements de décalage. La gate historique948 reste inchangée.
Ces deux suites passent dans les captures Release et instrumentée.

229cas de clés :170supports valides et59refus, donnant63boules distinctes
dont10présentes dans les trois arités. q2 :64valides/2refus ; q3 :48/23 ;
q4 :58/34. Les170encodages représentent2891mots, minimum10 et maximum201
(40à804octets), hors objet24octets et allocateur. Ce corpus extrême n'est
pas une mesure de la taille moyenne sur SemanticKITTI.

960cas d'événements :41seeds invalides,398comparaisons coplanaires
refusées,521comparaisons valides donnant161ordres−1,202égalités et
158ordres+1. Tests CLI séparés :77refus clés et64refus événements,
code1 exact. Dix-huit corruptions de sorties sont détectées ; les
selftests natifs comptent52contrôles clés et689événements. La préparation
d'événements occupe184octets sur ces builds, sans allocation propre.

La régression précédente passe de nouveau :1636cas Fraction de boules,
53refus CLI,13corruptions de sorties,1287contrôles natifs dont128requêtes
sur supports partagés par quatre lecteurs. Les tests concurrents de
clés/événements sont également fonctionnels, pas une qualification TSan.

Les deux mutants recompilés omettent respectivement la translation
globale d'une clé et le signe du deuxième dénominateur q4. Les sondes
doivent retourner0 sans diagnostic : l'oracle exige précisément la
réponse géométrique fausse visée, tout en gardant l'autre primitive
correcte. Un crash, un échec de compilation ou un seul compteur incorrect
ne valent pas une détection géométrique. Référence et deux mutants sont
archivés séparément, sans modification des sources produit.

La compression de mots ne quantifie pas les coordonnées. Les tailles
publiées sont celles des objets ou encodages, pas du RSS ; les copies de
clé recopient leur vecteur. Aucun chrono de pipeline ni nouvelle mesure
de croissance8k/16k/32k n'est dérivé de ces tests de primitives.

## Relecture et reproduction

Les quatre [relectures communes](READBACK.json) normal/−O concordent,
ainsi que les quatre [relectures de mutations](MUTATION_READBACK.json),
avec et sans contrôle vivant supplémentaire. Les lecteurs rejugent les
oracles et corruptions ; ils ne relancent pas les exécutables natifs.

- Clôture Release : `94a76a4f5662e4a67cbeda7890a735b9d477d0587572054500aabbd2b28b6f6f`.
- Clôture ASan/UBSan : `c8af60628af2957662479d7b7905f9bc3f8b6dd20f7a31a82810564e21169adb`.
- Clôture mutations : `077df93fa88c9806347c99413f24756ffd3bb2213aa91282c7e840bb063ac341`.

```bash
python -B morsehgp3D_v8/bench/run_float32_identity_checks.py read --path morsehgp3D_v8/receipts/float32_identity_20260921/release/identity_0ecgriwi
python -B -O morsehgp3D_v8/bench/run_float32_identity_checks.py read --path morsehgp3D_v8/receipts/float32_identity_20260921/sanitize/identity_4o1kodni
python -B -O morsehgp3D_v8/tests/float32_identity_mutations.py read --path morsehgp3D_v8/receipts/float32_identity_20260921/mutations --check-live
```

Pour reconstruire, employer `run --build NEUF --output NEUF --compiler
/usr/bin/g++` ; ajouter `--sanitize --compiler /usr/bin/clang++` pour le
runner instrumenté. Ne jamais réécrire les trois builds/captures épinglés.
