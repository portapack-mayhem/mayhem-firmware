"""Check source preservation and the world-map binary format."""
import contextlib
import importlib.util
import io
import os
from pathlib import Path
import struct
import tempfile
import unittest

from PIL import Image

spec = importlib.util.spec_from_file_location(
    "world_map", Path(__file__).with_name("generate_world_map.bin.py"))
world_map = importlib.util.module_from_spec(spec)
spec.loader.exec_module(world_map)


class WorldMapTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.source = self.root / "source.png"
        self.pixels = [(255, 0, 0), (0, 255, 0), (0, 0, 255),
                       (255, 255, 255), (0, 0, 0), (123, 45, 67)]
        im = Image.new("RGB", (2, 3))
        im.putdata(self.pixels)
        im.save(self.source)
        self.original = self.source.read_bytes()
        self.addCleanup(setattr, Image, "MAX_IMAGE_PIXELS", Image.MAX_IMAGE_PIXELS)

    def assert_alias_rejected(self, output):
        with self.assertRaisesRegex(ValueError, "different files"):
            world_map.convert_map(self.source, output)
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_same_path(self):
        self.assert_alias_rejected(self.source)

    def test_symlink(self):
        output = self.root / "alias.png"
        output.symlink_to(self.source)
        self.assert_alias_rejected(output)

    def test_hard_link(self):
        output = self.root / "alias.png"
        os.link(self.source, output)
        self.assert_alias_rejected(output)
        self.assertEqual(output.read_bytes(), self.original)

    def test_conversion_new_and_existing_output(self):
        expected = struct.pack("<HH", 2, 3)
        for r, g, b in self.pixels:
            expected += struct.pack("<H", ((r >> 3) << 11)
                                    | ((g >> 2) << 5) | (b >> 3))
        output = self.root / "map.bin"
        for existing in (False, True):
            with self.subTest(existing=existing):
                if existing:
                    output.write_bytes(b"old output" * 100)
                with contextlib.redirect_stdout(io.StringIO()):
                    world_map.convert_map(self.source, output, chunk_rows=2)
                self.assertEqual(output.read_bytes(), expected)
                self.assertEqual(self.source.read_bytes(), self.original)


if __name__ == "__main__":
    unittest.main()
