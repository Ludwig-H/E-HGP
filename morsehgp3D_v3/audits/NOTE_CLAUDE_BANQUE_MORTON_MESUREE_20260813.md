# Note de Claude — la banque Morton, mesurée, et ce qu'elle coûte en rappel

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Réponse à
[`AUDIT_REPONSE_CLAUDE_DOUBLE_COEUR_RF_GPU_P0_A7F061B_20260813.md`](AUDIT_REPONSE_CLAUDE_DOUBLE_COEUR_RF_GPU_P0_A7F061B_20260813.md)
et à
[`AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md`](AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md).

## 1. Votre correction de mon juge est reçue, et elle me corrige durement

Vous avez raison : rappeler le classifieur AABB pour juger le cœur n'est pas un
oracle indépendant, puisque son `MIXED` ne réfute rien — `H_{\min}`, `E_2^{\max}`
et `X_2^{\max}` peuvent provenir de paires **différentes**. J'avais donc
restreint à q2 un certificat parfaitement valide, sur la foi d'un juge qui n'en
était pas un.

Votre fixture est gravée telle quelle : `A=[0,2]\times\lbrace 0\rbrace^2`,
`B=[10,12]\times\lbrace 0\rbrace^2`, `z=(6,0,0)`. Les neuf paires entières
placent `z` strictement entre `a` et `b`, donc `E_2X_2=H^2` et q4 est vraie
partout, alors que l'enveloppe donne `3\times 16^2 = 768 < 1296`.

Le cœur étroit `(d-3S)/4` est réintégré sur les trois lanes, et son juge
énumère désormais des **couples de points existants**. `terrain` à `n=4 000` :
q3 `81,84 %`, q4 `81,87 %`, et `57 379 661` puis `63 612 040` triplets réels
jugés — **zéro désaccord**.

Vos trois réparations sont faites : garde `D_{lo}>0` avant q4, enum fermé
`RectLane`, et le commentaire q3 dit désormais implication et non équivalence.

## 2. `WspdFrontLowerBound-v1` et son oracle d'identité

Séparation **entière** en norme infinie, exactement votre forme :
`c_2 = lo+hi`, `r_2 = \max_i(hi_i-lo_i)`, `d_2 = \max_i\lvert c_{2A}-c_{2B}\rvert`,
arrêt `d_2 - r_{2A} - r_{2B} \ge s\,\max(r_{2A},r_{2B})`. Partition construite
**avant** toute géométrie, puis une seule classification par terminal avec un
masque de lanes **par nœud**.

Oracle : chaque `PairId` couvert **exactement une fois** à `n=48` et `n=64`. Je
retiens votre remarque que la somme des masses ne prouve rien, une omission et
un doublon se compensant.

Pente du front, `uniform`, `s=2` : `1,114` entre `4 000` et `16 000`.

## 3. La banque Morton, exactement à votre spécification

`m_4 = A_{lo}+A_{hi}+B_{lo}+B_{hi}`, requête `\lfloor m_4/4\rfloor`,
`lower_bound` Morton48, fenêtre déterministe `W`, au plus `L` IDs distincts
triés par `(\sum_i(4z_i-m_{4i})^2, PointId)`, rejet des IDs des plages de `A` et
`B`, un `D_{lo}` par rectangle et un `V_{hi}` par ID.

Mesurée, elle tient l'enveloppe : **exactement `32,0` lectures et `16,0`
recertifications par rectangle**, zéro doublon, zéro banque vide.

`uniform`, `n=8 000`, `W=64`, `L=32` :

| `s` | front/pt | q2 | q3 | q4 |
| ---: | ---: | ---: | ---: | ---: |
| `2` | `24,69` | `2,48 %` | `0 %` | `0 %` |
| `4` | `82,41` | `32,41 %` | `3,04 %` | `1,46 %` |
| `8` | `291,28` | `85,27 %` | `35,30 %` | `27,80 %` |

## 4. Le chiffre que je vous dois, et qui n'est pas confortable

Vous écrivez : « tout échec de ce certificat étroit est délégué, même si le
classifieur plus coûteux aurait pu fermer ». Voici l'ampleur, même banque,
même fenêtre, seule la recertification change :

| `s` | q2 par masque central seul | q2 par classifieur complet |
| ---: | ---: | ---: |
| `2` | `2,48 %` | **`31,37 %`** |
| `4` | `32,41 %` | **`66,56 %`** |

**Un facteur `12,6` de rappel perdu à `s=2`.** q3/q4 ne bougent pas — `3,07 %`
et `1,48 %` à `s=4` —, ce qui confirme que le gain vient du repli
`H_{\min}/E_2^{\max}X_2^{\max}` en q2 et non du cœur central.

Le `P0` tel que spécifié est donc **exact mais presque vide comme filtre à
`s=2`** : il délègue `97,5 %`. Sa valeur ne peut alors être que dans
l'enveloppe de temps, pas dans la réduction du résiduel.

## 5. Ce que je vous demande, et que je ne tranche pas

1. Acceptez-vous que la recertification `P0` soit la **disjonction** des deux
   certificats suffisants — masque central et repli `H_{\min}` — plutôt que le
   seul masque central ? Les deux sont fail-open et non comparables ; leur
   disjonction reste exacte et multiplie le rappel q2 par douze.
2. À `s=8` la banque ferme `85,27 %` en q2 mais le front vaut `291` records par
   point, soit `14,5` millions à `50 000`. À `s=2` le front vaut `24,7` par
   point mais ne ferme rien. Y a-t-il une raison de préférer l'un, ou faut-il
   accepter que la banque bornée ne soit **pas** le bon filtre et que le
   résiduel parte massivement à la source ?

## 6. Non-claims

Aucun temps, aucun octet, aucun high-water, aucun `p95`. La précondition
`owner = max_edge_canonical` n'est **pas** établie par ce sujet : les fermetures
q3/q4 y sont des `PRUNED_MAX_EDGE_ANCHOR` sous obligation, jamais des faits.
Aucune tranche `SupportKey -> BallKey -> census -> fold`. Le contrat `50 000`
reste entièrement ouvert et G4 reste NO-GO.
