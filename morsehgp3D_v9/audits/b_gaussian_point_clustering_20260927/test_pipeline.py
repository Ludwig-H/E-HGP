#!/usr/bin/env python3
"""Small real-native end-to-end smoke and prepared-input corruption guards.

No benchmark scene, source edit, rebuild, cloud call, or clustering tuning.
All temporary input/output files live under a fresh TemporaryDirectory.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import numpy as np

from condensed import point_clusterer_from_tree, validate_condensed_tree
from evaluation import evaluate_labels, tree_recoverability, condensed_recoverability
import run as runner
from eom import condense_eom
from projection import SourceTree

NATIVE = None


def fixture():
    offsets = np.asarray([[0,0,0], [2,3,5], [5,1,4], [1,6,2], [7,4,1],
                          [3,2,8], [4,8,6], [8,7,3], [6,5,9]], dtype="<u4")
    centers = np.asarray([[100,100,100], [300,150,120], [160,350,250]], dtype="<u4")
    return np.concatenate([center+offsets for center in centers]), np.repeat([1,2,3],9).tolist()


def make_case(directory):
    directory.mkdir()
    points, truth = fixture()
    paths = dict(points_u32le=directory/"points.u32le", points_npy=directory/"points.npy",
                 labels_json=directory/"labels.json", parameters_json=directory/"parameters.json",
                 bayes_map_json=directory/"bayes_map.json")
    with paths["points_u32le"].open("xb") as out:
        out.write(points.tobytes())
    with paths["points_npy"].open("xb") as out:
        np.save(out, points.astype(np.float64), allow_pickle=False)
    runner.save(paths["labels_json"], truth)
    runner.save(paths["parameters_json"], dict(n=27,communities=3,fixture=True))
    runner.save(paths["bayes_map_json"], dict(predictions_reconstructed_grid=truth,fixture=True))
    result = dict(n=27, communities=3, **{key:str(path) for key,path in paths.items()})
    result["prepared_sha256"] = {key:runner.sha(paths[key]) for key in ("points_u32le","points_npy","labels_json")}
    result["diagnostic_sha256"] = {key:runner.sha(paths[key]) for key in ("parameters_json","bayes_map_json")}
    return result


class PipelineTests(unittest.TestCase):
    def test_real_native_pipeline(self):
        if NATIVE is None:
            self.fail("--native PATH is required; this gate never silently skips the native smoke")
        self.assertEqual(runner.sha(NATIVE),runner.EXPECTED_BINARY)
        signatures = []
        with tempfile.TemporaryDirectory(prefix="mhgp9-gaussian-pipeline-") as temp:
            root = Path(temp)
            case = make_case(root/"input")
            points, binary, truth = runner.read_case(case)
            self.assertEqual(points.shape,(27,3))
            inputs = runner.checked_inputs(case)
            for k in (5,10):
                command = [str(NATIVE),"--input",case["points_u32le"],"--k",str(k),"--workers","1"]
                completed = subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False,timeout=60)
                self.assertEqual(completed.returncode,0,completed.stderr.decode(errors="replace"))
                self.assertEqual(completed.stderr,b"")
                native = json.loads(completed.stdout)
                self.assertEqual(native["point_count"],27)
                self.assertEqual(native["k"],k)
                np.testing.assert_array_equal(native["points"],binary)
                tree = SourceTree(native).project("first_coverage",1)["tree"]
                original_tree = copy.deepcopy(tree)
                for minimum in (2,5,10,100):
                    for z in (1,2):
                        clustered = point_clusterer_from_tree(tree,min_cluster_size=minimum,exp_z=z)
                        legacy = condense_eom(tree["n"],tree["children"],tree["heights"],
                                              min_cluster_size=minimum,exp_z=z,atomize_ties=True)
                        self.assertEqual(clustered["selection"]["labels"],legacy["labels"])
                        self.assertEqual(clustered["selection"]["selected_legacy_ids"],legacy["selected"])
                        condensed = clustered["condensed_tree"]
                        validation = validate_condensed_tree(condensed)
                        self.assertEqual(validation["point_exits"],27)
                        self.assertEqual(sorted(condensed["point_exit_ids"]),list(range(27)))
                        self.assertEqual(len(condensed["point_exit_parent"]),27)
                        self.assertEqual(len(clustered["selection"]["labels"]),27)
                        self.assertEqual(clustered["stats"]["points_removed_from_input"],0)
                        self.assertEqual(clustered["stats"]["selected_points"]+clustered["stats"]["noise_points"],27)
                        if minimum == 100:
                            self.assertEqual(clustered["selection"]["labels"],[-1]*27)
                        evaluation = dict(labels=evaluate_labels(truth,legacy["labels"],min_cluster_size=minimum),
                            raw=tree_recoverability(truth,tree),
                            raw_size_filtered=tree_recoverability(truth,tree,min_cluster_size=minimum),
                            condensed=condensed_recoverability(truth,condensed))
                        for name,value in (("clustered",clustered),("evaluation",evaluation)):
                            path = root/f"k{k}_m{minimum}_z{z}_{name}.json"
                            runner.save(path,value)
                            saved = json.loads(path.read_text(),parse_constant=lambda x: self.fail("nonstandard JSON "+x))
                            self.assertEqual(saved,json.loads(json.dumps(runner.clean(value),allow_nan=False)))
                        signatures.append(dict(k=k,min_cluster_size=minimum,exp_z=z,
                            labels_sha256=hashlib.sha256(json.dumps(legacy["labels"]).encode()).hexdigest(),
                            point_exits=validation["point_exits"]))
                self.assertEqual(tree,original_tree)
            self.assertEqual(inputs,runner.checked_inputs(case))
        print(json.dumps(dict(pipeline="passed",native_commands=2,n=27,selections=signatures),sort_keys=True),flush=True)

    def test_read_case_rejects_corruption(self):
        def rewrite(case,key,value,rehash=True):
            path = Path(case[key])
            if key == "points_u32le":
                with path.open("wb") as out:
                    out.write(np.asarray(value,dtype="<u4").tobytes())
            elif key == "points_npy":
                with path.open("wb") as out:
                    np.save(out,np.asarray(value,dtype=np.float64),allow_pickle=False)
            else:
                runner.save(path,value)
            if rehash:
                namespace = "prepared_sha256" if key in case["prepared_sha256"] else "diagnostic_sha256"
                case[namespace][key] = runner.sha(path)

        def mutate(case,kind):
            points,truth = fixture()
            if kind == "binary_hash":
                points[0,0] += 1
                rewrite(case,"points_u32le",points,False)
            elif kind == "geometry_mismatch":
                points[0,0] += 1
                rewrite(case,"points_npy",points)
            elif kind == "truth_length":
                rewrite(case,"labels_json",truth[:-1])
            elif kind == "truth_zero":
                truth[0] = 0
                rewrite(case,"labels_json",truth)
            elif kind == "truth_float":
                truth[0] = 1.0
                rewrite(case,"labels_json",truth)
            elif kind == "missing_class":
                case["communities"] = 4
            elif kind in ("duplicate_points","outside_u18"):
                if kind == "duplicate_points":
                    points[1] = points[0]
                else:
                    points[0,0] = 2**18
                rewrite(case,"points_u32le",points)
                rewrite(case,"points_npy",points)
            elif kind in ("parameters_hash","bayes_hash"):
                key = "parameters_json" if kind == "parameters_hash" else "bayes_map_json"
                rewrite(case,key,dict(corruption=True),False)
            else:
                case["n"] = 26

        kinds = ("binary_hash","geometry_mismatch","truth_length","truth_zero","truth_float",
                 "missing_class","duplicate_points","outside_u18","parameters_hash","bayes_hash","wrong_n")
        with tempfile.TemporaryDirectory(prefix="mhgp9-gaussian-corruption-") as temp:
            for kind in kinds:
                with self.subTest(kind=kind):
                    case = make_case(Path(temp)/kind)
                    runner.read_case(case)
                    mutate(case,kind)
                    with self.assertRaises(ValueError):
                        runner.read_case(case)
        print(json.dumps(dict(read_case_corruptions=len(kinds),status="rejected"),sort_keys=True),flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native",type=Path,required=True)
    args, remaining = parser.parse_known_args()
    NATIVE = args.native.resolve()
    unittest.main(argv=[sys.argv[0],*remaining])
