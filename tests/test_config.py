"""Run with an ESPHome-enabled Python: python -m unittest discover -s tests."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
BASE = f"""esphome:
  name: nicera-validation
rtl87xx:
  board: generic-rtl8720cf-2mb-896k
external_components:
  - source:
      type: local
      path: {ROOT / 'components'}
    components: [nicera_np624m]
binary_sensor:
  - platform: nicera_np624m
    name: Motion
    serial_in_pin: PA2
    direct_link_pin: PA3
"""


class ConfigTests(unittest.TestCase):
    def test_motion_settings(self):
        with tempfile.TemporaryDirectory(prefix="nicera-schema-") as directory:
            config = Path(directory) / "config.yaml"
            cases = {
                "ignore_time": ("500ms", "0.5s", "1500ms", "8s", "0ms", "499ms", "750ms", "8500ms"),
                "pulse_count": ("1", "2", "3", "4", "0", "5", "1.5", "-1"),
                "window_time": ("2s", "4s", "6s", "8s", "0s", "3s", "9s", "2500ms"),
            }
            for key, values in cases.items():
                for index, value in enumerate(values):
                    with self.subTest(setting=key, value=value):
                        config.write_text(BASE + f"    {key}: {value}\n")
                        result = subprocess.run(
                            [sys.executable, "-m", "esphome", "config", str(config)],
                            capture_output=True, text=True,
                        )
                        output = result.stdout + result.stderr
                        if index < 4:
                            self.assertEqual(result.returncode, 0, output)
                        else:
                            self.assertNotEqual(result.returncode, 0, output)
                            self.assertIn(key, output)

    def test_pin_modes(self):
        with tempfile.TemporaryDirectory(prefix="nicera-schema-") as directory:
            config = Path(directory) / "config.yaml"
            for key, pin in (("serial_in_pin", "PA2"), ("direct_link_pin", "PA3")):
                for mode in (None, "OUTPUT", "INPUT", "INPUT_PULLUP", "OUTPUT_OPEN_DRAIN"):
                    with self.subTest(pin=key, mode=mode):
                        value = pin if mode is None else f"{{number: {pin}, mode: {mode}}}"
                        config.write_text(BASE.replace(f"{key}: {pin}", f"{key}: {value}"))
                        result = subprocess.run(
                            [sys.executable, "-m", "esphome", "config", str(config)],
                            capture_output=True, text=True,
                        )
                        output = result.stdout + result.stderr
                        if mode in (None, "OUTPUT"):
                            self.assertEqual(result.returncode, 0, output)
                        else:
                            self.assertNotEqual(result.returncode, 0, output)
                            self.assertIn("PIR protocol pins must use push-pull OUTPUT mode", output)


if __name__ == "__main__":
    unittest.main()
