"""Benchmark samples contain at least three seconds of measured work."""

import argparse
import math
import random


def minimum_seconds(value):
    """The argument parser accepts finite minimum durations of at least three seconds."""
    try:
        seconds = float(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError('minimum duration must be a number') from error
    if not math.isfinite(seconds) or seconds < 3:
        raise argparse.ArgumentTypeError('minimum duration must be at least 3 seconds')
    return seconds


def measure_case(names, execute, work, repetitions, seconds=3.0, odd_work=False):
    """The case uses common work counts and discards samples below the minimum duration."""
    if not math.isfinite(seconds) or seconds < 3 or repetitions < 1 or work < 1:
        raise ValueError('invalid benchmark measurement configuration')
    if odd_work and work % 2 == 0:
        raise ValueError('the initial work count must be odd')
    names = list(names)
    if not names or len(set(names)) != len(names):
        raise ValueError('benchmark implementations must be nonempty and unique')
    randomizer = random.Random(20261003)
    calibration = []

    def run(name):
        sample = execute(name, work)
        duration = sample['seconds']
        if not math.isfinite(duration) or duration <= 0:
            raise RuntimeError(f'{name}: invalid measured duration {duration}')
        return sample

    def increase(duration):
        estimate = work * (seconds * 1.2 / duration)
        if not math.isfinite(estimate) or estimate > (1 << 63) - 1:
            raise RuntimeError('benchmark calibration exceeds the supported work count')
        next_work = max(work + 1, math.ceil(estimate))
        if odd_work and next_work % 2 == 0:
            next_work += 1
        if next_work > (1 << 63) - 1:
            raise RuntimeError('benchmark calibration exceeds the supported work count')
        return next_work

    while True:
        order = list(names)
        randomizer.shuffle(order)
        warmup = {name: run(name) for name in order}
        fastest = min(sample['seconds'] for sample in warmup.values())
        calibration.append({'work': work, 'seconds': {name: sample['seconds'] for name, sample in warmup.items()}})
        if fastest < seconds:
            work = increase(fastest)
            continue

        samples = {name: [] for name in names}
        run_order = []
        short_sample = None
        for repetition in range(repetitions):
            sequence = order[repetition % len(order):] + order[:repetition % len(order)]
            for name in sequence:
                sample = run(name)
                if sample['seconds'] < seconds:
                    short_sample = {'implementation': name, 'seconds': sample['seconds'], 'work': work}
                    break
                samples[name].append(sample)
                run_order.append(name)
            if short_sample is not None:
                break
        if short_sample is None:
            return {'work': work, 'samples': samples, 'run_order': run_order,
                    'calibration': calibration, 'minimum_sample_seconds': seconds}
        calibration.append({'discarded_sample': short_sample})
        work = increase(short_sample['seconds'])
