import pytest
import importlib.util
import sys
import json
from unittest.mock import patch, MagicMock

# Load the script as a module despite the unusual filename
spec = importlib.util.spec_from_file_location(
    "generate_ui_about",
    "firmware/tools/generate_ui-about-simple.cpp.py"
)
module = importlib.util.module_from_spec(spec)

@pytest.mark.parametrize("payload", [
    # Exact exploit: malicious JSON injecting C++ code via MITM
    '[{"login": "x\"; system(\\"rm -rf /\\"); //", "avatar_url": "http://evil.com"}]',
    # Boundary: empty contributor list
    '[]',
    # Valid input: normal contributor data
    '[{"login": "validuser", "avatar_url": "https://avatars.example.com/u/1"}]',
])
def test_https_enforced_for_external_fetch(payload):
    """Invariant: External data fetches MUST use HTTPS with certificate verification enabled."""
    mock_response = MagicMock()
    mock_response.text = payload

    captured_calls = []

    def mock_get(url, **kwargs):
        captured_calls.append({"url": url, "kwargs": kwargs})
        return mock_response

    with patch("requests.get", side_effect=mock_get):
        try:
            spec.loader.exec_module(module)
        except Exception:
            pass  # Script may fail due to missing env/files; we only care about the request call

    for call in captured_calls:
        url = call["url"]
        kwargs = call["kwargs"]
        # MUST use HTTPS
        assert url.startswith("https://"), (
            f"External fetch must use HTTPS, got: {url}"
        )
        # MUST NOT disable certificate verification
        assert kwargs.get("verify", True) is not False, (
            "Certificate verification must not be disabled (verify=False)"
        )