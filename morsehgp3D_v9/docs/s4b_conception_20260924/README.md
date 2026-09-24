# S4b : voie q4 sur l'appareil, sans atlas (conception, 24 septembre 2026)

```text
phase=exploration_v9_hors_registre
backend=reference_cpu (+ cuda_g4 à qualifier)
profile=quantized_u18_input_only
mode=conception_puis_port
public_status=not_claimed
```

**GCP non utilisé pour la conception.** Un workflow a produit trois conceptions
indépendantes (A « pivot », B « lentilles », C « fenêtre restreinte »), chacune
avec un prototype hôte. Une synthèse jugée les a départagées. Les prototypes
étaient hors dépôt et ne constituent pas des reçus. Toutes les mesures citées
portent sur la trame sans sol 08/000000 à s = 8.

## Décision

La base est la conception B, **par lentilles**. Pour chaque graine possédée et
strictement aiguë de l'arête (les mêmes graines que q3, S4a) :

1. **Domaine** (L2) : toute racine émise est dans $[-\bar\mu,\bar\mu]$, avec
   $2\bar\mu^2\geq Q$ et $Q=D(3G-2EF)$. C'est le lemme de la lentille,
   $\alpha_4=2$.
2. **Passe de lentilles** (L1) : on découpe le domaine en J = 8 seaux sur une
   grille entière symétrique. Un site négatif aux deux bouts d'un seau est
   intérieur à tout membre du seau. Quand les huit comptes de lentille
   atteignent T = K−2, la graine est **certifiée vide**. Le contrôle a lieu
   après chaque paquet de 32 sites, dans l'ordre en anneaux de S4a.
3. **Survivants** : les événements des seaux vivants sont tamponnés. Les
   candidats sont les sites tels que $P>0$, possédés et canoniques (L3, L7).
   Dans chaque seau vivant, le candidat de plus petit ID fixe un groupe par
   une **forme pivot** (L5). Cette forme, $\det$ et $num$, est le numérateur de
   `make_q4` ; elle donne aussi la positivité et la clé. On obtient la
   profondeur (lentille + sorties au-dessus + entrées au-dessous, L4) et le
   groupe entier. Le plus petit ID positif du groupe est émis. La propriété des
   racines aux bornes est réglée par L6.

Les lemmes L1–L8 et la suffisance du cover pour q4 sont inscrits au
[registre des preuves](../../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md),
section V9-S4. Il n'y a aucun flottant, et la fenêtre $[L,U]$ de Window30
n'est pas reproduite.

## Écarts assumés de la v1 (commit S4b)

La synthèse prévoyait trois étapes que la v1 repousse, parce que l'objet n'en
dépend pas :

- **Pas de raffinement récursif** des seaux vivants (R = ∞). Le prototype
  sans raffinement est égal au moteur ; seule la traîne d'une graine lourde
  augmente.
- **Une arête par warp**, comme S4a : pas de tâches (arête, 32 graines). Le
  même warp enchaîne le prologue, les recensements q3 puis les graines q4.
- **Clé calculée en ligne** par la voie meneuse, sans noyau de clés séparé.

On mesure d'abord ; tâches et raffinement suivent si la traîne domine sur G4.

## Mesures (hôte, 08/000000)

| | K5 | K10 |
| --- | ---: | ---: |
| arêtes q4 | 576 456 | 1 357 994 |
| graines | 7 126 317 | 31 928 291 |
| certifiées vides / dès le premier paquet | 6 253 220 / 5 338 976 | 27 437 764 / 20 885 337 |
| paquets de la passe | 17 312 783 | 106 636 691 |
| tétraèdres émis (égaux au moteur, arête par arête) | 158 496 | 1 732 548 |

Pour comparaison, une passe sur tout le cover pour chaque graine demanderait
580,7 M paquets à K5.

La v1 est égale à la voie q4 du moteur (Local28) arête par arête sur **toutes**
les arêtes de 08/000000, à K5 comme à K10. La chaîne avec le levier
`q34_batch_q4` reproduit les condensés épinglés. Ce premier essai a aussi
révélé un défaut de port, corrigé : un drapeau « valide » laissé d'un groupe
au suivant produisait des supports dupliqués. La chaîne l'a refusé
(`chain_duplicate_presentation`) avant toute publication.

## Portes

- `mhgp9_gpu_lanes_port` : q4 arête par arête contre `engine_q4_records`
  (familles, fixture cosphérique à groupes de 75 sites, fixture multi-groupes
  de l'auditeur B) ; lot mixte q3 + q4 passé à `check_lanes_batch` et au juge ;
  tampon d'événements réduit ; mutants q4.
- `mhgp9_chain_batch_q3` : bras S4b jugé, et bras à ardoise et tampon
  réduits ; mêmes condensés, même registre des voies.
- Contrat sonde/lecteur v21 : cas q4 réel, jugé, et mutants.
