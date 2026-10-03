# Preuves et réponses aux huit questions du développeur

3 octobre 2026. Questions publiées en 8df2025ab, SHA 9a1003e1.
Cadre `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.
Preuves de modèle mathématique et gardes Python bornées ; aucun build,
exécutable natif, fit HDBSCAN ni action GCP dans ce reçu.

- **Q1–Q3**, `tower_math/README.md` : chaîne complète depuis les inclusions
  géométriques, compositions exactes d'entrelacement, couverture qualifiée
  et stabilité 3ε ; poids fixes et contre-insertion ; portée de l'optimum
  intrinsèque ; retard≤d_k/2 à partir de t′. 77 gardes normal/−O.
- **Q4**, `evidence_head/Q4_REPONSE.md` : extraits primaires du chapitre7,
  borne finie de perte, transfert asymptotique CONDITIONNEL à une masse
  critique négligeable, inclusions/encadrement pour la fermeture. Pas de
  conclusion inconditionnelle à K/contraste fixes.
- **Q5**, preuve déjà close dans `../hm_followup_20261003/check_vertical.py`,
 55 gardes : deux ordres qualifiés se croisent sur six sites collinéaires.
- **Q6–Q7/S**, `geometry_catalogue/README.md` : construction par cardinalité
  qui réfute l'impossibilité générale formulée, sans recommandation de modèle ;
  relecture de S et complément géométrique en rayon pour les dates d'ER0h.
 6113 gardes exactes, normal/−O ; sources des cibles datées épinglées.
- **Q8**, `root/Q8_CONTRAT.md` : type de date proposé, comparateur quatre
  racines, classes de carrés et borne de séparation de six racines ; budgets
  selon u18/u21/u24 et limites des types/formats actuels. 12824 gardes scalaires,
  normal/−O ; bornes extrêmes conservatrices, pas de Cloud atteignant ces bornes.
- **Plateaux**, `owner_plateau/README.md` : contre-garde de l'oracle ab200
  en Decimal120, k2/m1, date exactement égale à une fusion ; mauvais enfant
  dans l'oracle, parent correct dans le helper exact 457. 42 gardes normal/−O.
  Les sources ont ensuite changé : aucun verdict sur ce nouveau correctif.
- **Correctif**, `owner_fix_review/README.md` : oracle 2f05, rival et owner
  décidés exactement ; la contre-garde est soldée.20 831 gardes normal/−O,
  dont 4 349 comparaisons indépendantes et invariances sur le nuage.
  Aucune qualification native/G4 et aucun transfert depuis pts4.

La réponse active reste une seule note dans audits/, mise à jour en place.
Les ledgers des sous-groupes restent fermés, celui du présent dossier
inventorie tous ses payloads. Les états de travail du développeur et les
sources effectivement consommées par G4 sont distingués des sources relues.

Reproduction, depuis le groupe correspondant :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -B check.py
PYTHONDONTWRITEBYTECODE=1 python3 -O -B check.py
# geometry_catalogue : check_answers.py ; root : check_date_bounds.py
# owner_plateau : check_owner.py
# owner_fix_review : check_fix.py
sha256sum -c SHA256SUMS
```

Limites : résultats de gardes bornées, aucune qualification native nouvelle,
aucun contrat 100 ms/200 ms, GPU ou multi-millions ; stabilité de dates distincte
de stabilité d'IoU et de sélection. Une insertion/suppression n'est pas le
métrique d'appariement conservant tous les identifiants. La constante 3 est
sharp abstraitement, sans sharp géométrique démontré.

Les budgets Q8 sont des majorants conservateurs. En particulier, le majorant
204 bits du numérateur u24 dépasse les 192 bits du format exporté ; aucune
fixture de nuage atteignant ce majorant n'est présentée. La complétude du
format u24 demande un format élargi ou une réduction prouvée.

Erratum documentaire du reçu précédent : ses deux copies de `points_gate.py`
portent toutes deux eb467b91 (SHA256SUMS fait autorité), malgré la mention
cc56 dans son README. Elles capturent le WIP enrichi à la copie ; cela ne
qualifie aucune session G4 et ne change pas les preuves mathématiques.
