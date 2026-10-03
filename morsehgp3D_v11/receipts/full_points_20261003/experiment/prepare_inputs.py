"""Prepare the five complete Zoltan inputs outside Git, preserving raw PointIds.

Only data hashes and public scene/target metadata are saved in the experiment.
No hierarchy, model, native executable or network operation is invoked.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

SCENES = ("01_velos_en_rang", "02_velos_contre_facade",
          "03_pieton_contre_facade", "04_velos_en_rang_avec_sol",
          "05_temoin_voitures_en_file")
MAX_OUTPUT_BYTES = 13_000_000


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2,
                       allow_nan=False) + "\n").encode()


def write_or_equal(path, body):
    if path.exists():
        require(path.is_file() and not path.is_symlink() and path.read_bytes() == body,
                "existing_output_differs:" + str(path))
    else:
        with path.open("xb") as handle:
            handle.write(body)


def prepare(source_cache, demos, output, inventory_path):
    import numpy as np

    require(not Path(output).is_symlink(), "output_directory_symlink")
    cache, demos, output = (Path(p).resolve() for p in (source_cache, demos, output))
    repo = Path(__file__).resolve().parents[4]
    for forbidden in (repo, Path("/workspaces/E-HGP")):
        require(not output.is_relative_to(forbidden), "data_output_inside_repository")
    output.mkdir(parents=True, exist_ok=True)
    rows, output_bytes, sources = [], 0, []
    for name in SCENES:
        specification_path = demos / name / "demo.json"
        published_path = demos / name / "resultats_hdbscan_K5.json"
        specification = json.loads(specification_path.read_text())
        published = json.loads(published_path.read_text())
        scene_path = cache / "scenes" / name / "scene.json"
        scene = json.loads(scene_path.read_text())
        seq, frame = specification["seq"], specification["frame"]
        expected_ground = "none" if name == SCENES[3] else "patchwork_v8"
        require(specification["ground"] == scene["ground"] == expected_ground,
                "scene_ground_mismatch:" + name)
        require(scene["seq"] == seq and scene["frame"] == frame, "scene_identity:" + name)
        raw_path = cache / "velodyne" / f"{seq}_{frame}.bin"
        labels_path = cache / "labels" / f"{seq}_{frame}.label"
        raw, labels_raw = raw_path.read_bytes(), labels_path.read_bytes()
        require(sha256(raw) == specification["sha256"] == scene["velodyne_sha256"],
                "raw_frame_hash:" + name)
        require(sha256(labels_raw) == specification["labels_sha256"] == scene["labels_sha256"],
                "raw_labels_hash:" + name)
        n_raw = scene["n_raw_frame"]
        require(type(n_raw) is int and len(raw) == 16 * n_raw and len(labels_raw) == 4 * n_raw,
                "whole_frame_length:" + name)
        xyz_raw = np.frombuffer(raw, dtype="<f4").reshape(n_raw, 4)
        require(bool(np.isfinite(xyz_raw[:, :3]).all()), "raw_xyz_nonfinite:" + name)
        original_labels = np.frombuffer(labels_raw, dtype="<u4")
        mask_path, mask_sha = None, None
        if expected_ground == "none":
            kept = np.arange(n_raw, dtype=np.uint32)
        else:
            mask_path = cache / "ground" / f"{seq}_{frame}.mask"
            mask = mask_path.read_bytes()
            mask_sha = sha256(mask)
            require(mask_sha == scene["ground_mask_sha256"] and len(mask) == n_raw,
                    "ground_mask_hash_length:" + name)
            mask_values = np.frombuffer(mask, dtype=np.uint8)
            require(bool((mask_values <= 2).all()), "ground_mask_domain:" + name)
            kept = np.flatnonzero(mask_values != 1).astype(np.uint32)
        n = scene["n_sites"]
        require(type(n) is int and n == scene["n_points"] == len(kept)
                and scene["duplicates_merged"] == 0, "scene_bijection_required:" + name)
        flat = cache / "criblage" / "g4"
        sites_path = flat / f"zoltan_{name}_sites.u32le"
        truth_path = flat / f"zoltan_{name}_truth.u32le"
        sites, truth_body = sites_path.read_bytes(), truth_path.read_bytes()
        require(len(sites) == 12 * n and sha256(sites) == scene["sites_u32le_sha256"],
                "sites_hash_length:" + name)
        require(len(truth_body) == 8 * n, "truth_length:" + name)
        xyz = np.frombuffer(sites, dtype="<u4").reshape(n, 3)
        require(bool((xyz < 2 ** 18).all()), "observed_u18_domain:" + name)
        truth = np.frombuffer(truth_body, dtype="<i4").reshape(2, n)
        require(bool((truth[0] >= -2).all()) and bool(((truth[1] >= -1) & (truth[1] <= 2)).all()),
                "truth_domain:" + name)
        npz_path = cache / "scenes" / name / "scene.npz"
        with np.load(npz_path, allow_pickle=False) as stored:
            inverse = stored["raw_to_site"]
            require(inverse.shape == (n,) and inverse.dtype.kind in "iu"
                    and bool(np.array_equal(np.sort(inverse), np.arange(n))),
                    "raw_to_site_not_bijection:" + name)
            require(np.array_equal(stored["sites"], xyz), "cached_sites_differ:" + name)
            require(np.array_equal(stored["X"], xyz_raw[kept, :3].astype(np.float64)),
                    "retained_original_xyz_differ:" + name)
            require(np.array_equal(stored["L"], original_labels[kept]),
                    "retained_original_labels_differ:" + name)
            id_by_site = np.empty(n, dtype="<u4")
            id_by_site[inverse] = kept
            label_by_site = np.empty(n, dtype="<u4")
            label_by_site[inverse] = stored["L"]
            object_by_site = np.empty(n, dtype="<i4")
            object_by_site[inverse] = stored["obj"]
            require(np.array_equal(truth[1], object_by_site), "tracked_objects_differ:" + name)
            void = np.isin(label_by_site & 0xFFFF, (0, 1, 52, 99))
            require(np.array_equal(truth[0] == -2, void), "truth_void_differ:" + name)
            seen_labels = set()
            groups = np.unique(truth[0][truth[0] >= 0])
            require(np.array_equal(groups, np.arange(len(groups))), "truth_group_ids_not_contiguous:" + name)
            for group in groups:
                group_labels = np.unique(label_by_site[truth[0] == group])
                require(len(group_labels) == 1 and int(group_labels[0]) not in seen_labels,
                        "truth_group_not_one_original_instance:" + name)
                seen_labels.add(int(group_labels[0]))
        require(n == published["n_points_hierarchy"] and int(void.sum()) == published["void_points"],
                "published_population_mismatch:" + name)
        targets = []
        for index, obj in enumerate(specification["objects"]):
            selected = obj["select"]
            expected = ((label_by_site & 0xFFFF) == selected["sem"]) & ((label_by_site >> 16) == selected["inst"])
            require(np.array_equal(expected, truth[1] == index), "target_identity:" + name)
            count = int(expected.sum())
            require(count == published["objects"][index]["points"], "target_count:" + name)
            targets.append(dict(target=index, key=obj["key"], sem=selected["sem"],
                                inst=selected["inst"], sites=count))
        files = []
        for suffix, body in (("sites", sites), ("ids", id_by_site.tobytes()), ("truth", truth_body)):
            filename = f"zoltan_{name}_{suffix}.u32le"
            output_bytes += len(body)
            require(output_bytes <= MAX_OUTPUT_BYTES, "output_data_budget")
            write_or_equal(output / filename, body)
            files.append(dict(role=suffix, name=filename, bytes=len(body), sha256=sha256(body)))
        source_paths = [specification_path, published_path, scene_path, raw_path, labels_path,
                        sites_path, truth_path, npz_path] + ([] if mask_path is None else [mask_path])
        sources.extend(dict(path=str(p), bytes=p.stat().st_size, sha256=sha256(p.read_bytes()))
                       for p in source_paths)
        rows.append(dict(name=name, seq=seq, frame=frame, ground=expected_ground,
                         whole_frame=True, subsampling="none", n_raw=n_raw, n=n,
                         sites=n, duplicates_merged=0, void_sites=int(void.sum()),
                         numeric_profile="quantized_u21_input_only; observed coordinates within u18",
                         grid_m="1/1000", point_ids="original zero-based raw frame return index, permuted to site order",
                         truth_layout="i32[n] all group labels/background-1/void-2; i32[n] tracked ABC-1/0/1/2",
                         raw_sha256=sha256(raw), labels_sha256=sha256(labels_raw),
                         ground_mask_sha256=mask_sha, targets=targets, files=files,
                         published_hdbscan_k5_best_iou=[o["best_iou"] for o in published["objects"]]))
    manifest = dict(schema="mhgp11.full_points.input_manifest.v1", scene_count=len(rows),
                    scenes=rows, data_bytes=output_bytes,
                    preparation_scope="input integrity and ID mapping only; no hierarchy qualification")
    manifest_body = canonical(manifest)
    require(output_bytes + len(manifest_body) <= MAX_OUTPUT_BYTES, "output_data_manifest_budget")
    write_or_equal(output / "manifest.json", manifest_body)
    expected_names = {file["name"] for row in rows for file in row["files"]} | {"manifest.json"}
    require({path.name for path in output.iterdir()} == expected_names,
            "unexpected_output_file_inventory")
    inventory = dict(schema="mhgp11.full_points.public_inventory.v1", scenes=rows,
                     data_directory=str(output), manifest_sha256=sha256(manifest_body),
                     data_bytes=output_bytes, max_data_bytes=MAX_OUTPUT_BYTES,
                     source_files=sources, sklearn_required_version="1.7.2",
                     native_profile="u21", regimes="five complete Zoltan development scenes; four frames of sequence08",
                     hierarchy_diagnostic="atomic plateaus; best IoU is an oracle diagnostic, not automatic selection",
                     points_and_annotations_in_git=False)
    write_or_equal(Path(inventory_path), canonical(inventory))
    return inventory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-cache", type=Path,
                        default=Path("/workspaces/E-HGP/build/v10-lidar-demos/_cache"))
    parser.add_argument("--demos", type=Path, default=Path("/workspaces/E-HGP/Zoltan/demos"))
    parser.add_argument("--out", type=Path, default=Path("/workspaces/.ehgp-data/full-points-20261003"))
    parser.add_argument("--inventory", type=Path, default=Path(__file__).with_name("inventory.json"))
    args = parser.parse_args()
    value = prepare(args.source_cache, args.demos, args.out, args.inventory)
    print(json.dumps(dict(status="prepared", scenes=len(value["scenes"]),
                          data_bytes=value["data_bytes"], manifest_sha256=value["manifest_sha256"])))


if __name__ == "__main__":
    main()
