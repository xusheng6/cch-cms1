"""Validation of DOS no-best-move evidence, independent of engine execution."""
import unittest

from compare_cms1_oracle import has_no_best_move


class NoBestMoveTests(unittest.TestCase):
    def test_empty_root_list(self):
        self.assertTrue(has_no_best_move({"pv_words": [], "root_move_bytes": 0}))

    def test_exhausted_roots(self):
        trace = {"pv_words": [], "root_move_bytes": 12, "root_score": -9656,
                 "root_moves": [{"source": 65535}, {"source": 65535}]}
        self.assertTrue(has_no_best_move(trace))
        for change in ({"root_moves": [{"source": 6}, {"source": 65535}]},
                       {"root_moves": []}, {"root_move_bytes": 6},
                       {"root_score": 0}, {"pv_words": [6, 132]}):
            self.assertFalse(has_no_best_move(trace | change))

    def test_missing_evidence(self):
        self.assertFalse(has_no_best_move({"pv_words": []}))


if __name__ == "__main__":
    unittest.main()
