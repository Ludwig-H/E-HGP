#!/usr/bin/env python3
"""Fabrique le recu publiable d'une session G4 de la v12 : resultats choisis, sans identite de compte.

Usage : recu_session.py --session DOSSIER_DE_SESSION --dest DOSSIER_DU_RECU [--include MOTIF]...
  - copie receipt.json en masquant recovery_command et recovery (qui portent l'adresse du compte) ;
  - copie les fichiers de results/extracted/results qui correspondent a un motif --include (fnmatch sur le chemin
    relatif, par exemple 'commands.tsv', 'env/vm_facts.txt', 'cmd/*/files/**') ;
  - dans chaque fichier texte copie, remplace le dossier personnel de la VM (/home/<nom>) par $HOME et l'adresse du
    compte gcloud (lue dans preflight.json, jamais ecrite) par <compte> ;
  - ecrit SHA256SUMS, puis verifie qu'aucun fichier du recu ne contient plus ni l'adresse, ni le nom POSIX, ni
    '/home/'. Jamais copies : preflight.json, host/, package/, results.tar.gz, session.stdout et session.stderr.
Le dossier de destination doit etre absent ou vide : il n'est jamais efface s'il preexistait (CST-0221) ; en cas
d'identite restante, seuls les fichiers crees par l'appel sont retires. Les noms de fichiers sont expurges comme les
contenus, et le controle final porte sur les noms et sur tous les fichiers, SHA256SUMS compris (CST-0219).
Bibliotheque standard seule. Codes : 0 conforme ; 2 usage ; 3 identite restante (fichiers crees retires).
"""

import argparse
import fnmatch
import hashlib
import json
import os
import re
import shutil
import sys

HOME_RE = re.compile(r'/home/[A-Za-z0-9_.-]+')


def redact_text(text, account, user):
  text = HOME_RE.sub('$HOME', text)
  if account:
    text = text.replace(account, '<compte>')
  if user:
    text = text.replace(user, '<utilisateur>')
  return text


def is_text(path):
  with open(path, 'rb') as handle:
    head = handle.read(4096)
  return b'\0' not in head


def main(argv):
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  parser.add_argument('--session', required=True)
  parser.add_argument('--dest', required=True)
  parser.add_argument('--include', action='append', default=[])
  try:
    args = parser.parse_args(argv[1:])
  except SystemExit:
    return 2
  receipt_path = os.path.join(args.session, 'receipt.json')
  results = os.path.join(args.session, 'results', 'extracted', 'results')
  if not os.path.isfile(receipt_path) or not os.path.isdir(results):
    print('recu_session : session incomplete (receipt.json ou resultats absents)', file=sys.stderr)
    return 2
  account = ''
  try:
    with open(os.path.join(args.session, 'preflight.json'), encoding='utf-8') as handle:
      account = json.load(handle).get('gcloud_account', '') or ''
  except (OSError, ValueError):
    pass
  user = ''
  match = None
  for root, _, files in os.walk(results):
    for name in files:
      path = os.path.join(root, name)
      if is_text(path):
        with open(path, encoding='utf-8', errors='replace') as handle:
          match = HOME_RE.search(handle.read())
        if match:
          break
    if match:
      break
  if match:
    user = match.group(0).split('/')[2]
  if os.path.exists(args.dest) and (not os.path.isdir(args.dest) or os.listdir(args.dest)):
    print('recu_session : la destination existe et n\'est pas vide ; rien n\'est ecrit', file=sys.stderr)
    return 2
  created_dest = not os.path.exists(args.dest)
  os.makedirs(args.dest, exist_ok=True)
  with open(receipt_path, encoding='utf-8') as handle:
    receipt = json.load(handle)
  for key in ('recovery_command', 'recovery'):
    if key in receipt:
      receipt[key] = '<masque : porte l\'adresse du compte>'
  text = redact_text(json.dumps(receipt, indent=1, sort_keys=True), account, user)
  with open(os.path.join(args.dest, 'receipt.json'), 'w', encoding='utf-8') as out:
    out.write(text + '\n')
  copied = 0
  for root, _, files in os.walk(results):
    for name in sorted(files):
      path = os.path.join(root, name)
      relative = os.path.relpath(path, results)
      if not any(fnmatch.fnmatch(relative, pattern) for pattern in args.include):
        continue
      target = os.path.join(args.dest, 'resultats', redact_text(relative, account, user))
      os.makedirs(os.path.dirname(target), exist_ok=True)
      if is_text(path):
        with open(path, encoding='utf-8', errors='replace') as handle:
          content = redact_text(handle.read(), account, user)
        with open(target, 'w', encoding='utf-8') as out:
          out.write(content)
      else:
        shutil.copyfile(path, target)
      copied += 1
  needles = [n for n in (account, user, '/home/') if n]
  sums = []
  for root, _, files in os.walk(args.dest):
    for name in sorted(files):
      path = os.path.join(root, name)
      relative = os.path.relpath(path, args.dest)
      if relative == 'SHA256SUMS':
        continue
      with open(path, 'rb') as handle:
        sums.append('%s  %s' % (hashlib.sha256(handle.read()).hexdigest(), relative))
  with open(os.path.join(args.dest, 'SHA256SUMS'), 'w', encoding='utf-8') as out:
    out.write('\n'.join(sorted(sums, key=lambda line: line.split('  ', 1)[1])) + '\n')
  leaks = []
  for root, _, files in os.walk(args.dest):
    for name in sorted(files):
      path = os.path.join(root, name)
      relative = os.path.relpath(path, args.dest)
      with open(path, 'rb') as handle:
        data = handle.read()
      if any(n in relative for n in needles) or any(n.encode() in data for n in needles):
        leaks.append(relative)
  if leaks:
    if created_dest:
      shutil.rmtree(args.dest)
    else:
      for root, dirs, files in os.walk(args.dest, topdown=False):
        for name in files:
          os.remove(os.path.join(root, name))
        for name in dirs:
          os.rmdir(os.path.join(root, name))
    print('recu_session : identite restante dans %d fichier(s) ; fichiers crees retires' % len(leaks), file=sys.stderr)
    return 3
  print('recu_session_ok fichiers=%d' % (copied + 1))
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
