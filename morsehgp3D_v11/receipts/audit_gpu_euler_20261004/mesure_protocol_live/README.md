# Correctifs des protocoles de mesure — source publiée

Pin `61da03749344a6acc4fea2b9eee875606cc857e8`, incluant correctif `d5b1d0179`.
Quatre blobs Git copiés avant lecture et recoupés après ; l'acteur était encore
au pin77, explicitement distinct de la source jugée. Aucun produit modifié,
aucun programme natif, build, fit ou GCP exécuté.

**Les trois réserves principales sont fermées en source.**

- `sources/ab_g4.py:91–107,205–222` : ordre Williams ; N=2 donne AB puis BA.
  Les cycles complets équilibrent positions et successions dirigées. Avec
  cinq répétitions/N=2, le programme annonce `balanced=false` ; aucune portée
  équilibrée ne se transmet rétrospectivement aux campagnes anciennes.
- `sources/ab_summary.py:60–71,79–103,110–119` : le dérivé conserve l'empreinte
  exacte des octets du parent, verdict/refus, identités, plan, builds et archives.
  Code non nul, statut non-ok ou dump différent sont exclus. Les nombres de
  paires attendues/retenues et la p minimale sont présents. Code0 reste «lu»,
  même lorsque le parent est refusé. Les contrôles du parent (dont quiescence
  et exit) restent autorité : ce dérivé n'est pas une qualification autonome.
- `sources/full_timing.py:50–55,127–149` : TimeoutExpired devient une prise
  timeout avec diagnostics bornés ; le banc continue et checkpoint chaque
  prise achevée. Le rapport final garde l'échec et retourne1. Provenance XYZ/IDs
  et note descriptive des tranches sont conservées.

Rejeu **237 gardes**, normal/−O strictement identiques : Williams N0..12 ;
quatre rapports appariés synthétiques ; trois appels subprocess **simulés**
succès/timeout/succès, quatre checkpoints observés par le véritable main.
Le helper vérifie le hash avant extraction AST/exécution des seules définitions
Python publiées. Il ne lance jamais le main A/B ni de processus natif.
`sources/full_pipeline_reader_test.py` est une pièce corrective lue, non exécutée.

Nuance de représentation : lorsque les deux bras d'une même répétition sont
exclus, cette répétition manque à `rows[].dropped`, construit depuis les prises
retenues. Les deux exclusions globales et expected5/pairs4 la rendent néanmoins
visible ; aucun nouveau faux verdict conforme n'est produit. Ne considérer
pas `dropped` seul comme inventaire exhaustif des absences.

```sh
python3 -B -S check.py
python3 -O -B -S check.py
python3 -B -S verify.py
python3 -O -B -S verify.py
```

`BEFORE.json`/`AFTER.json` distinguent sources Git et état de l'acteur.
`COMMANDS.json` conserve aussi une recherche auxiliaire initiale en échec,
sans exécution perdue. `SHA256SUMS` inventorie tous les fichiers réguliers de
cette capsule, sauf ce manifeste racine lui-même. Aucun chrono natif ni gain
nouvellement qualifié.
