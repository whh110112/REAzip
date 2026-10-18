"""Exercise release rejection paths without uploading or changing GitHub state."""
import copy
import hashlib
import importlib.util
import json
import pathlib
import tempfile
import unittest
from unittest.mock import patch

path = pathlib.Path(__file__).resolve().parents[2] / 'tools/publish-native.py'
spec = importlib.util.spec_from_file_location('publish_native', path)
publisher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publisher)


class PromotionValidation(unittest.TestCase):
    def setUp(self):
        self.work = tempfile.TemporaryDirectory()
        self.addCleanup(self.work.cleanup)
        self.root = pathlib.Path(self.work.name)
        self.source = 'a' * 40
        formats = {'windows': ['-Setup.exe', '-Portable.zip'], 'macos': ['.dmg', '.zip'],
                   'linux': ['.AppImage', '.deb', '.tar.gz']}
        self.manifests = []
        for system, suffixes in formats.items():
            for arch in ('x64', 'arm64'):
                prefix = f'ZENzip-99.1.2-{system}-{arch}'
                artifacts = []
                for suffix in suffixes:
                    file = self.root / (prefix + suffix)
                    payload = (system + arch + suffix).encode()
                    file.write_bytes(payload)
                    artifacts.append(dict(file=file.name, size=len(payload), sha256=hashlib.sha256(payload).hexdigest()))
                manifest = self.root / (prefix + '-manifest.json')
                manifest.write_text(json.dumps(dict(os=system, architecture=arch, version='99.1.2',
                    sourceCommit=self.source, sourceDirty=False, artifacts=artifacts)))
                self.manifests.append(manifest)

    def validate(self):
        return publisher.candidates(self.root, '99.1.2', self.source)

    def edit(self, **changes):
        file = self.manifests[0]
        data = json.loads(file.read_text())
        data.update(changes)
        file.write_text(json.dumps(data))

    def test_all_six_targets_and_twenty_assets(self):
        self.assertEqual(len(self.validate()), 20)

    def test_missing_target_is_rejected(self):
        self.manifests[0].unlink()
        with self.assertRaises(AssertionError): self.validate()

    def test_foreign_source_is_rejected(self):
        self.edit(sourceCommit='b' * 40)
        with self.assertRaises(AssertionError): self.validate()

    def test_dirty_source_is_rejected(self):
        self.edit(sourceDirty=True)
        with self.assertRaises(AssertionError): self.validate()

    def test_wrong_version_is_rejected(self):
        self.edit(version='99.1.1')
        with self.assertRaises(AssertionError): self.validate()

    def test_corrupted_package_is_rejected(self):
        data = json.loads(self.manifests[0].read_text())
        (self.root / data['artifacts'][0]['file']).write_bytes(b'corrupt')
        with self.assertRaises(AssertionError): self.validate()

    def test_missing_format_is_rejected(self):
        data = json.loads(self.manifests[0].read_text())
        self.edit(artifacts=data['artifacts'][:-1])
        with self.assertRaises(AssertionError): self.validate()

    def test_unexpected_package_path_is_rejected(self):
        data = copy.deepcopy(json.loads(self.manifests[0].read_text()))
        data['artifacts'][0]['file'] = '../outside.exe'
        self.edit(artifacts=data['artifacts'])
        with self.assertRaises(AssertionError): self.validate()

    def test_draft_is_found_without_the_published_tag_endpoint(self):
        draft = dict(id=7, tag_name='v99.1.2', draft=True)
        def api(*arguments, **kwargs):
            # Mirror GitHub's observed draft lookup behavior, not an existing
            # published release. Calling the tag endpoint would fail here.
            if arguments == ('api', 'repos/example/project/releases?per_page=100'):
                return [draft]
            raise RuntimeError('Draft is not visible through the tag endpoint')
        with patch.object(publisher, 'gh', side_effect=api):
            self.assertEqual(publisher.find_release('example/project', 'v99.1.2'), draft)

    def test_absent_draft_returns_none(self):
        with patch.object(publisher, 'gh', return_value=[]):
            self.assertIsNone(publisher.find_release('example/project', 'v99.1.2'))


if __name__ == '__main__':
    unittest.main()
