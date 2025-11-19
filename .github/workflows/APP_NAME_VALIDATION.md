# App Name Length Validation

This validation ensures that all app names in the Mayhem firmware conform to the display length requirements.

## Requirements

- **RX/TX locations**: Maximum 8 characters
- **All other locations (HOME, UTILITIES, GAMES, etc.)**: Maximum 12 characters

## Components

### 1. Build-time Validation (`make_spi_image.py`)

The app name validation is integrated into the build process in `firmware/tools/make_spi_image.py`. It runs automatically during firmware compilation and will display warnings if any app names exceed the length limits.

**Location**: `firmware/tools/make_spi_image.py`

### 2. Pull Request Validation (GitHub Actions)

A GitHub Actions workflow automatically checks app names in pull requests that modify:
- External app files (`firmware/application/external/*/main.cpp`)
- Standalone app files (`firmware/standalone/*/main.cpp`)
- Built-in app definitions (`firmware/application/ui_navigation.cpp`)

**Files**:
- Workflow: `.github/workflows/check_app_names.yml`
- Script: `.github/workflows/check_app_names.py`

## How It Works

### Checked Locations

1. **External Apps**: Scans all `main.cpp` files in `firmware/application/external/*/`
2. **Standalone Apps**: Scans all `main.cpp` files in `firmware/standalone/*/`
3. **Built-in Apps**: Parses the `NavigationView::appList` array in `firmware/application/ui_navigation.cpp`

### Validation Rules

The validator extracts two key fields from each app:

```cpp
/*.app_name = */ "AppName",
/*.menu_location = */ app_location_t::RX,
```

And checks:
- If `menu_location` is `RX` or `TX`: app_name length must be ≤ 8 characters
- For all other locations: app_name length must be ≤ 12 characters

## Running Locally

### Using the standalone script:

```bash
# Text output
python3 .github/workflows/check_app_names.py --repo-root . --format text

# Markdown output (for PR comments)
python3 .github/workflows/check_app_names.py --repo-root . --format markdown
```

### During build:

The validation runs automatically when building firmware and will display warnings in the build output.

## GitHub Actions Behavior

When a PR is opened or updated with changes to app files:

1. The workflow checks all app names
2. Results are posted as a comment on the PR
3. If violations are found:
   - The check fails (red X)
   - A detailed table shows all violations
   - The comment is updated on subsequent pushes
4. If all names are valid:
   - The check passes (green checkmark)
   - A success message is posted

## Example Output

### Passing Case
```
✅ PASSED: All app names are within length limits!
```

### Failing Case
```
❌ FAILED: Found app names exceeding length limits

| App Name | Location | Current Length | Max Length | File |
|----------|----------|----------------|------------|------|
| `Analog TV` | RX | 9 | 8 | `firmware/application/external/analogtv/main.cpp` |
| `Antenna Length` | UTILITIES | 14 | 12 | `firmware/application/external/antenna_length/main.cpp` |
```

## Fixing Violations

To fix violations, shorten the app name in the respective file:

**External/Standalone Apps** (`main.cpp`):
```cpp
/*.app_name = */ "ShortName",  // Must be ≤8 chars for RX/TX, ≤12 for others
```

**Built-in Apps** (`ui_navigation.cpp`):
```cpp
{"appid", "ShortName", RX, Color::green(), &icon, new ViewFactory<AppView>()},
//         ^^^^^^^^^^
//         Display name - must meet length requirements
```

## App Location Types

```cpp
enum app_location_t {
    UTILITIES = 0,
    RX,          // Max 8 characters
    TX,          // Max 8 characters
    DEBUG,
    HOME,
    SETTINGS,
    GAMES,
    TRX
};
```

## Troubleshooting

### Build warnings but no errors
The validation in `make_spi_image.py` only warns and doesn't fail the build. However, the GitHub Actions check will fail PRs.

### False positives
If you believe a validation is incorrect:
1. Check the app's `menu_location` field
2. Verify the character count (including spaces)
3. Report an issue if the validation logic is wrong

### Modifying the limits
To change the length limits, update:
- `get_max_length_for_location()` in `make_spi_image.py`
- `get_max_length_for_location()` in `check_app_names.py`
