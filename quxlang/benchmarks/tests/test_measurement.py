"""The regression tests verify minimum-duration calibration and matched work counts."""

import argparse
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from measurement import measure_case, minimum_seconds


class MeasurementTests(unittest.TestCase):
    def test_common_work_count_uses_fastest_implementation(self):
        def execute(name, work):
            return {'seconds': work * (0.01 if name == 'fast' else 0.02), 'work': work}

        result = measure_case(['fast', 'slow'], execute, 1, 3)
        self.assertGreater(result['work'], 1)
        for samples in result['samples'].values():
            self.assertEqual(len(samples), 3)
            self.assertTrue(all(sample['seconds'] >= 3 for sample in samples))
            self.assertTrue(all(sample['work'] == result['work'] for sample in samples))

    def test_short_measured_sample_restarts_the_whole_comparison(self):
        calls = 0

        def execute(name, work):
            nonlocal calls
            if work == 1:
                calls += 1
            return {'seconds': 1 if work == 1 and calls == 4 else 4, 'work': work}

        result = measure_case(['a', 'b'], execute, 1, 2)
        self.assertGreater(result['work'], 1)
        self.assertTrue(any('discarded_sample' in attempt for attempt in result['calibration']))
        for samples in result['samples'].values():
            self.assertEqual(len(samples), 2)
            self.assertTrue(all(sample['work'] == result['work'] for sample in samples))
            self.assertTrue(all(sample['seconds'] >= 3 for sample in samples))

    def test_replayed_work_remains_odd_after_calibration(self):
        result = measure_case(['a'], lambda name, work: {'seconds': work / 10}, 1, 2, odd_work=True)
        self.assertEqual(result['work'] % 2, 1)
        self.assertTrue(all(sample['seconds'] >= 3 for sample in result['samples']['a']))

    def test_minimum_is_configurable_above_three_seconds(self):
        result = measure_case(['a'], lambda name, work: {'seconds': work / 10}, 1, 1, seconds=5)
        self.assertGreaterEqual(result['samples']['a'][0]['seconds'], 5)

    def test_invalid_measured_durations_fail(self):
        for seconds in [0, -1, float('nan'), float('inf')]:
            with self.subTest(seconds=seconds), self.assertRaises(RuntimeError):
                measure_case(['a'], lambda name, work: {'seconds': seconds}, 1, 1)

    def test_calibration_overflow_fails(self):
        with self.assertRaisesRegex(RuntimeError, 'supported work count'):
            measure_case(['a'], lambda name, work: {'seconds': 1e-300}, 1, 1)

    def test_minimum_argument_rejects_short_or_nonfinite_durations(self):
        for value in ['0', '2.99', 'nan', 'inf', 'invalid']:
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                minimum_seconds(value)
        self.assertEqual(minimum_seconds('3'), 3)
        self.assertEqual(minimum_seconds('4.5'), 4.5)

    def test_invalid_configurations_fail(self):
        for names, work, repetitions, seconds, odd in [
            ([], 1, 1, 3, False), (['a', 'a'], 1, 1, 3, False),
            (['a'], 0, 1, 3, False), (['a'], 1, 0, 3, False),
            (['a'], 1, 1, 2, False), (['a'], 2, 1, 3, True),
        ]:
            with self.subTest(names=names, work=work, repetitions=repetitions, seconds=seconds, odd=odd):
                with self.assertRaises(ValueError):
                    measure_case(names, lambda name, count: {'seconds': 4}, work, repetitions, seconds, odd)


if __name__ == '__main__':
    unittest.main()
