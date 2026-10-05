# S7 — contrelecture native ciblée

Pin : `966a351be1b58089fd13bc44a8dbafc56d87999a` (sortie supports et corrections du relecteur léger). Le HEAD observé `00bd979ac` ajoute seulement le budget de matrice G4 ; les sources épinglées sont byte-identiques au pin. Cadre : exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed.

**Verdict : lecture favorable, aucun nouveau défaut important établi.** La hiérarchie et l’arbre sont possédés ensemble ; l’identité Session est gardée avant I/O ; la sérialisation paie seulement un tampon borné sur pile ; les colonnes suivent le format et ses bornes ; le manifeste et le lecteur recoupent les agrégats et la signature V2 ; les refus suivent la transaction et ne produisent aucun succès de porte par vacuité.

`source_manifest.json` ferme les SHA et distingue le pin des HEAD observés. `review.json` donne la portée réelle, les chemins causaux contrôlés et les limites ; cinq sources nouvelles/modifiées sont conservées dans `sources/`.

Aucune compilation, aucun test natif, aucune campagne GCP. Aucune qualification transférée, aucune nouvelle alarme sur les écarts G4 déjà déclarés, aucune note active modifiée.

`layout_bounds.py` rejoue les bornes de format et des agrégats du manifeste sur 3 896 formes. Depuis ce dossier :

```sh
python3 -S -B layout_bounds.py > layout_bounds.json
python3 -O -S -B layout_bounds.py > layout_bounds_opt.json
```

Codes 0, stderr vide et sorties identiques à l’octet (`layout_replay.json`). Aucune hypothèse ABI : les largeurs sont celles du format normatif.
