#!/usr/bin/env python3
"""Recoupe portable de métadonnées existantes seulement ; aucun import scientifique ou fit."""
from pathlib import Path
import hashlib, json
BASE = Path(__file__).resolve().parent
checks = 0
def need(ok, why):
    global checks
    checks += 1
    if not ok:
        raise ValueError(why)
def read(rel):
    return json.loads((BASE / rel).read_text())
before = read('SOURCE_BEFORE.json')
for row in before['sources']:
    if row.get('present') and row.get('sha256'):
        need(hashlib.sha256((BASE / row['payload']).read_bytes()).hexdigest() == row['sha256'], 'source snapshot: '+row['payload'])
eq = read('snapshot/equite/equivalence_out.json')
s = eq['stats']
need(s['configs'] == 6752, 'nombre configurations')
need(s['trees'] == 150, 'nombre arbres')
need(s['trees_with_plateau'] == 113, 'arbres plateaux')
need(s['i_eq_ii'] + s['i_ne_ii'] == s['configs'], 'partitions exhaustives i/ii')
need(s['i_eq_ii'] == 5737 and s['i_ne_ii'] == 1015, 'i/ii agrégé')
need(s['i_ne_ii_without_plateau'] == 0, 'écarts sans plateau')
need(s['fit_crash'] == 1672, 'bascules TypeError i')
need(s['canon_crash'] == 1560, 'bascules TypeError iii')
need(s['configs'] - s['fit_crash'] == s['fit_vs_tree'] == s['transcription_vs_fit'] == 5080, 'comparaisons sans TypeError i')
need(s['transcription_vs_fit_mismatch'] == 0, 'transcription/fit quand disponible')
need(s['ii_eq_iii'] + s['ii_ne_iii'] + s['canon_crash'] == s['configs'], 'comparaisons iii exhaustives')
need(s['ii_eq_iii'] == 5192 and s['ii_ne_iii'] == 0, 'iii disponible')
need(s['ii_eq_v'] == s['configs'] and s['ii_ne_v'] == 0, 'route v transcrite')
for key in ('by_k','by_family','by_method','by_eps'):
    need(sum(v['configs'] for v in eq[key].values()) == s['configs'], 'configurations '+key)
    need(sum(v['i_ne_ii'] for v in eq[key].values()) == s['i_ne_ii'], 'écarts '+key)
z, e = eq['by_eps']['zero'], eq['by_eps']['positive']
need(z == {'configs':2400,'fit_crash':0,'i_ne_ii':520}, 'eps zéro')
need(e == {'configs':4352,'fit_crash':1672,'i_ne_ii':495}, 'eps positif')
need(eq['sklearn'] == '1.9.1' and eq['numpy'] == '2.5.3', 'versions locales déclarées')
model = read('snapshot/modele/sorties/verif_eom_contre_sklearn.json')
need(model['clouds'] == 60 and model['forced'] == 0, 'pilotage EOM non forcé')
need(model['sans_ex_aequo']['checks'] == model['sans_ex_aequo']['agree'] == 135, 'EOM hors égalités')
need(model['avec_ex_aequo']['checks'] == 165 and model['avec_ex_aequo']['agree'] == 140 and model['avec_ex_aequo']['disagreements'] == 25, 'EOM égalités')
need(model['sans_ex_aequo']['checks'] + model['avec_ex_aequo']['checks'] == 300, 'total EOM')
need('lab_i = lab_iv' in (BASE/'snapshot/equite/equivalence.py').read_text(), 'imputation documentée présente')
need('elif force:' in (BASE/'snapshot/modele/scripts/modele_lib.py').read_text(), 'voie forced capturée')
need('iou.max(axis=1).sum()' in (BASE/'snapshot/mesure/metriques.py').read_text(), 'borne maxima capturée')
result = {'scope':'métadonnées des essais privés existants; aucune réexécution scientifique',
 'workflow':before['workflow'],'developer_head':before['developer_head'],
 'local_versions':{'sklearn':eq['sklearn'],'numpy':eq['numpy']},
 'g4_sklearn_comparator_version_prior':'1.7.2 (pas rejouée ici)',
 'plateau_comparison':{'configs_total':6752,'trees':150,'trees_with_plateau':113,
   'aggregated_i_ne_ii':1015,'i_ne_ii_without_plateau':0,
   'unambiguous_official_eps_zero':{'configs':2400,'differences':520,'typeerror_fallback':0},
   'epsilon_positive':{'configs':4352,'differences_aggregate':495,'typeerror_fallback':1672,
       'differences_among_real_fit_or_fallback_separately':'non déterminable dans ce JSON agrégé'},
   'official_fit_and_tree_successful_comparisons':5080,
   'normalized_official_code_available_comparisons':5192,
   'normalized_transcription_comparisons':6752,
   'typeerror_diagnostic':'exceptions non détaillées ; bloc try couvre fit puis tree_to_labels'},
 'model_eom_pilot':model,'checks':checks}
print(json.dumps(result,ensure_ascii=False,indent=2,sort_keys=True))
