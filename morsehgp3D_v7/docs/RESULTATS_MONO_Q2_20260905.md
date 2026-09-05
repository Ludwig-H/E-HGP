# Mono E : prétest q2 et comparaison WSPD s=8,10,12

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**Les trois paires conservent les objets et les comptes. Le gain total
observé est modeste et ne constitue pas un gain statistiquement qualifié.**
Le [reçu complet](../receipts/meb_q2_mono_20260905/README.md) conserve les
six sorties, commandes, coûts d'étages, usages mémoire et leur contrelecture.

## Même tour, mêmes plafonds

Uniforme n=8000, coordonnées u16 étendues (`coord=65536`), seed=3,
CPU logique 6, un thread, fold inline sérialisé, CSR, digest inclus et
aucune archive. Chaque run calcule la **tour candidate complétée K=1 à 10**
(`--complete-incidences`, `smax=11`), pas le seul préfiltre Gabriel.
Il ne livre toutefois ni verticale ni poids, et garde l'autorité
`normalized_horizontal_h0_candidate`.

Chaque s reçoit un nouveau D puis un nouveau E, bornés à 600 s et
26 GiB d'espace virtuel par processus. Aucun temps historique D n'est
réutilisé. Les plafonds de travail restent identiques. Les dix digests,
cardinalités, comptes généraux, comptes silent et plafonds sont identiques
dans chaque paire. Entre les trois s, les digests et cardinalités coïncident
aussi ; les comptes de travail WSPD n'ont pas à coïncider.

| s | D processus | E processus | Baisse observée | D complétion silent | E complétion silent |
|---|---:|---:|---:|---:|---:|
| 8 | 189,000 s | 184,178 s | 2,55 % | 72,601 s | 67,411 s |
| 10 | 192,556 s | 192,477 s | 0,04 % | 73,894 s | 69,184 s |
| 12 | 198,642 s | 192,730 s | 2,98 % | 74,813 s | 69,138 s |

Une seule paire froide ordonnée par s sur hôte partagé ne sépare pas
statistiquement l'effet du delta, celui de s et les variations de l'hôte.
À s=10, la baisse silent est presque entièrement compensée par des
étages plus lents. **Le défaut s=8 est conservé**, sans prétendre prouver
son optimalité. Aucun ratio C/E n'est reconstruit en mélangeant ces mesures
avec la campagne C/D historique.

## Interprétation et suite

E ajoute seulement le [prétest q2](OPTIMISATION_MEB_Q2.md) avant clé/niveau,
à charges et ordre de supports inchangés. Le
[rejeu rationnel indépendant](../audits/ADDENDUM_MEB_Q2_E_20260905.md)
confirme cette conservation locale ; les qualifications intégrées restent
une autorité distincte, décrite dans [PASSATION](../PASSATION.md).
Le CLI E mesuré a pour SHA-256
`df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6` ;
D reste `127c5f923fcc9618d826b89dedda4de0f5201ea48e27330e2ea68e83d76a1b3f`.

Le volume de complétion n'est pas réduit : 802 125 328 supports MEB,
581 904 257 visites de nœuds et 1 270 848 pas/ajouts. La tour conserve
4 384 229 événements et 26 434 998 facettes cumulées ; cette dernière
somme n'est pas un pic de résidence simultanée. À s=8, E dépense encore
environ 60,610 s en génération, 32,068 s en fold et 67,411 s en complétion.
La réduction du travail et des allocations doit donc continuer sur
plusieurs postes, sans changer l'objet pour obtenir un chrono inférieur.

La [proposition MEB par pivots](PROPOSITION_MEB_ET_BUDGETS.md) n'est pas
intégrée : son ordinal de référence ne borne pas son travail physique,
comme l'établit la contre-fixture conservée. La pile inline des témoins
reste elle aussi un candidat séparé, absent du binaire E mesuré.

Les cibles [50k/1 seconde puis 100 ms](CONTRAT_PERFORMANCE.md) et les
nuages de plusieurs dizaines de millions sur G4 ne sont pas atteints
ni extrapolés. Les mesures historiques 8k/16k/32k et G4 restent attribuées
à leurs propres binaires. GCP non utilisé pour ce jalon local.
