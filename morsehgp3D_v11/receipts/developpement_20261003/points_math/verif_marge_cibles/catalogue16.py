#!/usr/bin/env python3
"""Catalogue v2, n <= 16 (64 entrees), MON FULL rapide (aretes de Johnson) et MES regles ; deux juges."""
import sys, json
sys.dont_write_bytecode = True
import catalogue9 as C9
import indep_full_j as J
rules = sys.argv[1].split(',')
out = C9.run(rules, 16, J.FullJ)
json.dump(out, open('recus/catalogue16_%s.json' % '_'.join(rules), 'w'), indent=1)
print(json.dumps({k: v for k, v in out.items() if k != 'par_entree'}))
