"""Offline tests: no serial port is opened and no motor commands are sent."""
import importlib.util
import itertools
import pathlib
import unittest

GUI = pathlib.Path(__file__).parents[1] / "arduino-servo-control/ik_prototype/ik_calibration_gui.py"
spec = importlib.util.spec_from_file_location("arm_gui", GUI)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Variable:
    def __init__(self, value=None):
        self.value = value

    def set(self, value):
        self.value = value


class WideAutoTests(unittest.TestCase):
    def test_range_margin_and_repeatability(self):
        for pose in itertools.product(*module.DIRECT_LIMITS):
            for percent in (25, 50, 75, 100):
                poses = module.build_auto_sweep(pose, percent)
                self.assertEqual(poses, module.build_auto_sweep(pose, percent))
                self.assertEqual(len(poses), 8)
                for joint, (low, high) in enumerate(module.DIRECT_LIMITS):
                    values = [p[joint] for p in poses]
                    self.assertGreaterEqual(min(values), low + 5 - 1e-9)
                    self.assertLessEqual(max(values), high - 5 + 1e-9)
                    expected = (80, 50, 80, 90)[joint] * percent / 100
                    self.assertAlmostEqual(max(values) - min(values), expected)

    def test_entry_loop_and_seam_obey_speed_and_controller_duration(self):
        for pose in itertools.product(*module.DIRECT_LIMITS):
            poses = module.build_auto_sweep(pose)
            transitions = list(zip([pose] + poses, poses + [poses[0]]))
            for start, finish in transitions:
                for speed in (12, 18, 25):
                    duration = module.auto_move_duration_ms(start, finish, speed)
                    self.assertGreaterEqual(duration, 1800)
                    self.assertLessEqual(duration, 15000)
                    travel = max(abs(a - b) for a, b in zip(start, finish))
                    self.assertLessEqual(1.875 * travel / (duration / 1000), speed + 1e-9)

    def test_invalid_sweep_rejected(self):
        for percent in (0, 101, float("nan")):
            with self.assertRaises(ValueError):
                module.build_auto_sweep((33.5, 100, 140, 27.5), percent)

    def test_deferred_scale_callback_does_not_cancel_auto(self):
        app = module.IKCalibrationApp.__new__(module.IKCalibrationApp)
        app.manual_angle_text = [Variable() for _ in range(4)]
        app.manual_updating = False
        app.programmatic_slider_values = [33.7, 100, 150, 27.5]
        app.mark_user_activity = lambda source: self.fail("Programmatic callback counted as user")
        app.manual_slider_changed(0, "33.5")


if __name__ == "__main__":
    unittest.main()
