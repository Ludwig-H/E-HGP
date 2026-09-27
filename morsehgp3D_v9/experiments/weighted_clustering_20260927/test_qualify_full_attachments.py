"""Synthetic mutations of the reader; not native execution evidence."""
from copy import deepcopy
import unittest

from qualify_full_attachments import check_export, fixtures
from test_qualify_geometry import synthetic_pair, q


class AttachmentReaderTests(unittest.TestCase):
    def test_pair_and_mutations(self):
        fixture, weighted = synthetic_pair()
        payload = dict(schema='mhgp9_weighted_full_attachment_export_v1', status='completed',
                       weighted=weighted, anchors=[], validation=dict(anchors_available=False),
                       attachments=[dict(vertices=[i], beta=q(0), node=i,
                                         anchor_node=i, terminal_ball=None) for i in range(2)])
        result = check_export(payload, fixture)
        self.assertEqual(result['attached_facets'], 2)
        self.assertEqual(result['attachment_cut_queries'], 4)
        mutations = [lambda d: d['attachments'].pop(),
                     lambda d: d['attachments'].reverse(),
                     lambda d: d['attachments'][0].update(beta=q(1)),
                     lambda d: d['attachments'][0].update(node=2),
                     lambda d: d['attachments'][0].update(node=1, anchor_node=1),
                     lambda d: d['attachments'][0].update(anchor_node=True),
                     lambda d: d['attachments'][0].update(terminal_ball=0),
                     lambda d: d['anchors'].append(0),
                     lambda d: d['validation'].update(anchors_available=True)]
        for mutate in mutations:
            changed = deepcopy(payload)
            mutate(changed)
            with self.assertRaises(ValueError):
                check_export(changed, fixture)

    def test_e5_is_mandatory(self):
        cases = fixtures()
        self.assertEqual(len(cases), 33)
        self.assertEqual([row['k'] for row in cases if row['name'].startswith('e5_silent')], [1,2,3,4])


if __name__ == '__main__':
    unittest.main()
