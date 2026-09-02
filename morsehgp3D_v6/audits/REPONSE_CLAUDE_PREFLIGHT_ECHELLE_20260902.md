# RÉPONSE — préflight statique du profil G4 « échelle »

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Réponse point par point à `ALERTE_G4_ECHELLE_V6_20260902.md` (verdict NO
START sur `gcp-migration/profils/g4_echelle_v1.env`, coupe `8afd1057`). Le
verdict est accepté : aucune session ne part avant le raccord complet. GCP non
utilisé par cette réponse ; aucune VM n'existe.

## 1. Raccord local (vos P1)

1. **Profil absent des manifestes.** `g4_echelle_v1.env` est ajouté aux listes
   `PROTOCOL_FILES` de `v6_campaign_pin.sh` **et** de
   `v6_session_lifecycle.sh`, avec leurs portes — livré avec le point 2.
2. **Axe `smax` de bout en bout.** En cours de livraison : grammaire
   `famille:n[:smax]` (`smax` dans [2, 11], défaut 11), **normalisation** d'un
   `11` explicite sur l'absence (mêmes octets), nom d'artefact suffixé `_s6`
   hors K=10, `smax` porté dans le plan, l'argv, le `.status`, le résumé et le
   validateur, et **`frontier_plan=v2`** comme vous le demandez : `v1` reste
   émis à l'identique tant qu'aucun axe nouveau n'est présent, de sorte que les
   reçus antérieurs se revalident bit à bit. Les codes de
   `revalidate_v6_receipt.sh` sont mesurés avant et après sur
   `session_g4_20260901_d98f47296d67_1788245493` (avec frontière) et
   `session_g4_20260902_c8f696739b0b_1788312873` (sans).
3. **Layout gravé.** Le profil déclare `FRONTIER_LAYOUT="classic"` (baseline) ;
   le plan v2 porte le jeton, la commande passe `--layout=classic` et le
   validateur exige la correspondance exacte. Mesurer le palier KeyCSR n'est
   pas l'objet de cette session : sa campagne est pré-inscrite à part et
   différée.
4. **Code 124.** Votre lecture prévaut et le commentaire fautif du profil est
   corrigé : un timeout reste une **observation censurée**, jamais une donnée
   de frontière. La conclusion honnête d'une session tronquée est la frontière
   de complétion et le dernier profil réussi. Je ne demande aucun marqueur
   causal ; je réduis la matrice pour que la troncature soit improbable.

## 2. Portée scientifique (vos remarques)

- **Pente terrain à K=5** : la sécante 1 M → 2 M est remplacée par trois
  tailles, 200 000, 1 M et 2 M, appuyées sur les points déjà mesurés à 50 000.
- **Témoin apparié du pilote** : le pilote mesure désormais `uniform:50000`
  **et** `uniform:100000` sous le même commit et le même binaire. Sans ce
  témoin, aucune croissance « depuis 50k » ne serait opposable ; avec lui, le
  `digest_all` à 50 000 se compare aussi au reçu `1788293187`.
- **Étage qui déborde** : vous avez raison, un `bad_alloc` générique ne
  l'identifie pas. Je livre l'instrumentation correspondante dans le moteur :
  `run_pipeline` convertit un `bad_alloc` en refus transactionnel
  `resource_exhausted` **nommant l'étage atteint** et publiant les RSS
  d'étage, sans jamais publier de préfixe de payload, avec son mutant et sa
  porte à code exact. Tant qu'elle n'est pas reçue, la conclusion reste celle
  que vous écrivez : frontière de complétion et dernier profil réussi.

## 3. Budget et mémoire (vos chiffres)

Votre estimation (23 140 s crédités pour 23 995 s utiles, 855 s de marge) est
exacte contre la version que vous avez lue, et le commentaire du profil était
faux. La matrice est resserrée :

- frontière : **neuf** points au lieu de dix, plafond unitaire 1 200 s au lieu
  de 1 500 s, soit 10 800 s crédités ;
- device : build 900 s + portes 1 800 s = 2 700 s ;
- pilotes : deux points à 2 700 s = 5 400 s ;
- total crédité **≈ 19 010 s** pour une fenêtre utile de ≈ 23 995 s, soit
  ≈ 21 % de marge.

Le **préfixe obligatoire** est la frontière (Q1) ; les pilotes (Q2) sont un
**suffixe optionnel**. Une troncature ne détruit donc pas la question
décisionnelle.

`FRONTIER_ULIMIT_KB` passe de 183 500 800 (175 Gio) à **176 160 768**
(168 Gio), soit ≈ 8,9 Gio sous le `MemTotal` de 185 463 908 Kio gravé par le
reçu antérieur. Je retiens votre rappel : `RLIMIT_AS` ne borne pas le RSS du
système et ne protège pas de l'OOM killer ; la limite sert seulement à rendre
le refus **typé** dans le processus. `MemTotal`, `MemAvailable` et la charge
sont déjà gravés par le protocole et seront rapportés dans la note de reçu.

## 4. Ce que la session décide, et ce qu'elle ne décide pas

Elle décide : le plus grand `n` complet en mémoire à K=5 et à K=10 sur cette
machine, la pente du mur et du RSS sur quatre tailles par famille et par K, et
le coût de la couture hôte du pilote à 50 000 contre 100 000.

Elle ne décide pas : l'étage coupable au-delà de ce que l'instrumentation
ci-dessus rapporte, la validité des extrapolations à 10^7 points (aucune
mesure n'y prétend), ni aucun `public_status`.

Point d'appui local (compteurs déterministes, machine à huit fils, jamais un
mur) : `uniform` à K=5 donne 100 000 points en 5,21 Go et 200 000 en 10,37 Go,
soit 0,052 Mo par point et ≈ 83 boules par point. À 48 fils la résidence est
supérieure — c'est une des choses que la session mesure.
