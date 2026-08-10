# Audit live de reprise — mode cover et infrastructure de portes

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_differential`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype.
Le delta observé était encore non committé, au-dessus de
`f37341df99f64e2f1a43d6f85da78d1fb14fad31`; il doit donc être réaudité après
son commit. Le sous-snapshot cover testé avait les empreintes suivantes :

| objet | SHA-256 |
| --- | --- |
| `prototype/direct_source.cpp` | `f654ffddb2e5a86628ffe0517f01b4767d8af7bd5dc33a4b2ef192904dee74fa` |
| `CMakeLists.txt` | `1d0b7f6f1fd20126090ded1699afca0623517c720cabf7866fa77fd0d4709dc9` |
| binaire Release testé | `03ce7e105bb66e085fba532bcf2619d2a51973621c85be94dd8515d2f934cb2b` |
| diff binaire Git des deux fichiers contre `f37341d` | `e7baa5ee783c8d40db86de2b99a6960bfaadf4d409705603763f5576f0c1fbae` |

## Verdict

**GO fonctionnel borné pour la réparation du mode cover autonome. NO-GO pour
présenter les trois nouvelles portes comme résistantes aux mutations. Deux
dettes transversales préexistantes rendent en outre certaines portes négatives
et la qualification arithmétique moins fortes que leur libellé.**

Le correctif répond exactement au défaut de `84adbcc` : l'identité
`candidates == C_q` n'est plus exigée dans un mode qui n'énumère aucun
candidat, les planchers d'énumération sont refusés avant calcul, le rapport ne
simule plus une référence à temps nul et le disclaimer non qualifiant est
atteint.

Après configure et build Release frais dans `build/v3`, les trois nouvelles
portes passent :

```sh
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_direct_source_(cover_mode|cover_mode_disclaimer|reject_cover_floor)$'
```

Résultat : 3/3, zéro échec, 0,05 s. La sonde positive exacte

```sh
./build/v3/mhgp3v_direct_source --clouds 2 --points 100 --coord 50 --smax 11 --seed 7 --leaf 8 --judge 0 --cover-only 1 --min-clouds 2 --min-cover-tests 100000
```

retourne 0, décide 2/2 nuages, refuse zéro statut, compte 205 800 tests de
cover et imprime les deux disclaimers attendus. Avec `--min-emitted 1`, elle
retourne 2 avant tout calcul et annonce correctement l'incompatibilité.

L'ancienne sélection v3, configurée avant l'apparition de ces trois noms mais
exécutée sur le binaire reconstruit, passe séparément 70/70 en 614,30 s. Le
crédit honnête du delta est donc 70/70 puis 3/3, pas un run unique 73/73.

## P1 — les nouvelles portes restent partiellement vacuables

Trois mutations simples restent vertes par construction :

1. Le retour du vieux libellé temporel `catalogue seul — reference=0.000`
   laisserait la porte de code retour verte; la regex de la seconde porte vise
   seulement le disclaimer final `MODE COVER : ...`, pas la ligne
   `temps : MODE COVER`.
2. La condition de rejet couvre `--min-emitted`, `--min-windowed`,
   `--min-candidates` et `--min-forest-nodes`, mais le CTest n'exerce que le
   premier. Les trois autres branches ont été sondées positivement à la main,
   sans être gardées contre une régression.
3. Le plancher agrégé reçoit 205 800 tests, soit 68 600 pour chacune des lanes
   `q=2,3,4`. Omettre entièrement une lane laisse 137 200 tests et garde le
   seuil 100 000 vert. La porte prouve donc du travail agrégé, pas l'exercice
   des trois lanes.

Aide à Claude : faire porter la regex sur la ligne temporelle et le disclaimer,
paramétrer le rejet sur les quatre planchers, puis recevoir un compteur ou un
plancher par lane. Ces ajouts ne demandent aucune extension d'architecture.

## P1 — le harness d'échec accepte un crash comme succès

`cmake/expect_failure.cmake` refuse seulement un résultat numériquement égal à
zéro. `execute_process` rend une chaîne lorsqu'un processus meurt par signal;
la chaîne n'est pas zéro et le harness l'accepte si le texte attendu a déjà été
écrit. Reproduction :

```sh
cmake -DPROGRAM=/usr/bin/python3 -DEXPECTED=marqueur '-DARGUMENTS=-c;(__import__("os").write(1,b"marqueur\\n"),__import__("os").kill(__import__("os").getpid(),__import__("signal").SIGSEGV))' -P morsehgp3D_v3/cmake/expect_failure.cmake
```

Le harness retourne 0 et imprime `echec attendu confirme : code=Segmentation
fault`. Toutes les portes négatives qui l'emploient peuvent donc confondre un
rejet contractuel avec un crash précédé du bon diagnostic. Elles doivent exiger
un code de sortie numérique et, lorsque le contrat le fixe, sa valeur exacte.

## P2 — le self-test arithmétique conserve un parseur permissif

`oracle/bigint_selftest.cpp` lit encore son nombre de tours par `atoi` :

```sh
./build/v3/mhgp3v_arith_selftest 1junk
```

retourne 0, n'effectue que 20 vérifications et imprime `OK`. Le CTest courant
emploie la constante saine `20000`, donc son exécution n'est pas censurée; en
revanche, l'interface ne refuse ni suffixe ni arguments excédentaires et aucune
porte négative ne protège ce contrat. La même discipline `from_chars` déjà
appliquée aux autres binaires est attendue avant de présenter ce self-test
comme une qualification CLI robuste.

## Réponse live postérieure — palier du 10 août, 07:39 UTC

Claude a répondu positivement aux trois lacunes cover de ce sous-snapshot :

- `--min-lane-cover-tests 60000` reçoit séparément les lanes `q=2,3,4`, et la
  sortie publie leurs trois masses;
- la porte de libellés exige à la fois `temps : MODE COVER` et le disclaimer;
- quatre portes distinctes reçoivent les quatre planchers incompatibles, avec
  code exact 2.

Le harnais rejette maintenant tout `RESULT_VARIABLE` non numérique. La
reproduction `SIGSEGV` ci-dessus rend donc correctement le harnais rouge. Une
nouvelle macro transmet aussi `EXPECTED_CODE` aux portes arithmétiques et à
toutes les portes négatives de `direct_source`. La migration n'est pas encore
globale : les deux anciennes macros restent appelées par 23 portes oracle,
first-incidence, device et flats; elles rejettent désormais un signal mais
acceptent encore n'importe quel code numérique non nul. Une sonde qui imprime
`marqueur` puis rend 99 est verte sans `EXPECTED_CODE` et rouge avec
`EXPECTED_CODE=2`.

Enfin, `bigint_selftest` emploie maintenant `from_chars`, refuse suffixe, zéro
et argument excédentaire avec le code 2, et reçoit deux CTests négatifs à code
exact. Les sondes `1junk`, `1 2`, `0` rendent toutes 2; le témoin positif `1`
rend 0 avec GMP. Ces réponses ferment les findings cover et parsing du premier
sous-snapshot, ainsi que la confusion crash/rejet. La valeur exacte du code
reste une dette des 23 appels non migrés.

| objet live postérieur | SHA-256 |
| --- | --- |
| `prototype/direct_source.cpp` | `7a5d42e99814a493fd0fea872c49181301a383a32d3a24c6af0589d2e16ff652` |
| `CMakeLists.txt` | `72c1dd688f026bea42e491f9176c154a35a6c6dd1ba0f9d1832ded78ea74616d` |
| `cmake/expect_failure.cmake` | `8406dd02f501297e268688460a8e5e2abb4f964431f7425dfb4df973cceec56d` |
| `oracle/bigint_selftest.cpp` | `67417448772cdb954b4f3cdba4737b17e1ceffc7c7a36b5e04787e78880cd758` |

Les alertes distinctes sur le claim de payload public complet, les limites du
validateur topologique et les chronomètres vivent dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md). Ce palier non committé doit
encore être réépinglé.

GCP non utilisé.
