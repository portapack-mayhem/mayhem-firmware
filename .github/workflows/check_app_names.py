#!/usr/bin/env python3

"""
App Name Length Validator for Mayhem Firmware
Checks that app names conform to length requirements:
- RX/TX locations: max 8 characters
- Other locations: max 12 characters
"""

import re
import sys
from pathlib import Path
from typing import Dict, List, Optional


def parse_external_app_info(main_cpp_path: Path) -> Optional[Dict]:
    """Parse external app main.cpp file to extract app_name and menu_location"""
    try:
        with open(main_cpp_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Look for app_name field
        app_name_match = re.search(r'/\*\.app_name\s*=\s*\*/\s*"([^"]*)"', content)
        if not app_name_match:
            return None
        app_name = app_name_match.group(1)

        # Look for menu_location field
        menu_location_match = re.search(r'/\*\.menu_location\s*=\s*\*/\s*app_location_t::(\w+)', content)
        if not menu_location_match:
            return None
        location_str = menu_location_match.group(1)

        return {
            'app_name': app_name,
            'location': location_str,
            'file': main_cpp_path
        }
    except Exception as e:
        print(f"Error parsing {main_cpp_path}: {e}", file=sys.stderr)
        return None


def parse_standalone_app_info(main_cpp_path: Path) -> Optional[Dict]:
    """Parse standalone app main.cpp file to extract app_name and menu_location"""
    return parse_external_app_info(main_cpp_path)


def parse_builtin_apps(ui_navigation_cpp_path: Path) -> List[Dict]:
    """Parse ui_navigation.cpp to extract built-in app names and locations"""
    try:
        with open(ui_navigation_cpp_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Find the appList definition
        applist_match = re.search(
            r'const\s+NavigationView::AppList\s+NavigationView::appList\s*=\s*\{(.*?)\};',
            content,
            re.DOTALL
        )
        if not applist_match:
            print("WARNING: Could not find appList in ui_navigation.cpp", file=sys.stderr)
            return []

        applist_content = applist_match.group(1)

        # Parse each app entry
        # Pattern matches: {id, "DisplayName", LOCATION, Color, icon, factory}
        app_pattern = r'\{[^}]*?"([^"]+)",\s*(\w+),\s*[^}]*?\}'
        apps = []

        for match in re.finditer(app_pattern, applist_content):
            app_name = match.group(1)
            location_str = match.group(2)

            apps.append({
                'app_name': app_name,
                'location': location_str,
                'file': ui_navigation_cpp_path
            })

        return apps
    except Exception as e:
        print(f"Error parsing {ui_navigation_cpp_path}: {e}", file=sys.stderr)
        return []


def get_max_length_for_location(location_str: str) -> int:
    """Return maximum app_name length for given location"""
    if location_str in ['RX', 'TX']:
        return 8
    else:
        return 12


def validate_app_name_lengths(apps: List[Dict]) -> List[Dict]:
    """Validate app name lengths and return list of violations"""
    violations = []

    for app in apps:
        app_name = app['app_name']
        location = app['location']
        max_length = get_max_length_for_location(location)

        if len(app_name) > max_length:
            violations.append({
                'app_name': app_name,
                'location': location,
                'current_length': len(app_name),
                'max_length': max_length,
                'file': app['file']
            })

    return violations


def check_all_app_names(repo_root: Path) -> tuple[List[Dict], int]:
    """
    Check app names from external, standalone, and built-in apps
    Returns: (violations, total_apps_checked)
    """
    all_apps = []

    # Check external apps
    external_apps_dir = repo_root / "firmware" / "application" / "external"
    if external_apps_dir.exists():
        for main_cpp in external_apps_dir.glob("*/main.cpp"):
            app_info = parse_external_app_info(main_cpp)
            if app_info:
                all_apps.append(app_info)

    # Check standalone apps
    standalone_apps_dir = repo_root / "firmware" / "standalone"
    if standalone_apps_dir.exists():
        for main_cpp in standalone_apps_dir.glob("*/main.cpp"):
            app_info = parse_standalone_app_info(main_cpp)
            if app_info:
                all_apps.append(app_info)

    # Check built-in apps
    ui_navigation_cpp = repo_root / "firmware" / "application" / "ui_navigation.cpp"
    if ui_navigation_cpp.exists():
        builtin_apps = parse_builtin_apps(ui_navigation_cpp)
        all_apps.extend(builtin_apps)

    # Validate all apps
    violations = validate_app_name_lengths(all_apps)

    return violations, len(all_apps)


def format_output_text(violations: List[Dict], total_apps: int) -> str:
    """Format violations as text output"""
    output = []
    output.append("=" * 70)
    output.append("App Name Length Validation")
    output.append("=" * 70)
    output.append(f"Total apps checked: {total_apps}")
    output.append("")

    if violations:
        output.append("❌ FAILED: Found app names exceeding length limits:")
        output.append("")
        output.append("Requirements:")
        output.append("  - RX/TX locations: maximum 8 characters")
        output.append("  - Other locations: maximum 12 characters")
        output.append("")
        output.append("-" * 70)

        for v in violations:
            output.append(f"App: '{v['app_name']}'")
            output.append(f"  Location: {v['location']}")
            output.append(f"  Length: {v['current_length']} characters (max: {v['max_length']})")
            output.append(f"  File: {v['file']}")
            output.append("")

        output.append("-" * 70)
        output.append(f"Total violations: {len(violations)}")
    else:
        output.append("✅ PASSED: All app names are within length limits!")

    output.append("=" * 70)
    return "\n".join(output)


def format_output_markdown(violations: List[Dict], total_apps: int) -> str:
    """Format violations as GitHub markdown for PR comments"""
    output = []
    output.append("## App Name Length Validation Results")
    output.append("")
    output.append(f"**Total apps checked:** {total_apps}")
    output.append("")

    if violations:
        output.append("### ❌ Validation Failed")
        output.append("")
        output.append("The following app names exceed the maximum length requirements:")
        output.append("")
        output.append("**Requirements:**")
        output.append("- RX/TX locations: maximum **8 characters**")
        output.append("- Other locations: maximum **12 characters**")
        output.append("")
        output.append("### Violations")
        output.append("")
        output.append("| App Name | Location | Current Length | Max Length | File |")
        output.append("|----------|----------|----------------|------------|------|")

        for v in violations:
            # Make file path relative for better display
            file_path = str(v['file']).replace(str(Path.cwd()), '.')
            output.append(
                f"| `{v['app_name']}` | {v['location']} | "
                f"{v['current_length']} | {v['max_length']} | `{file_path}` |"
            )

        output.append("")
        output.append(f"**Total violations:** {len(violations)}")
        output.append("")
        output.append("Please shorten the app names to meet the requirements.")
    else:
        output.append("### ✅ Validation Passed")
        output.append("")
        output.append("All app names are within the required length limits!")

    return "\n".join(output)


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description='Validate app name lengths in Mayhem firmware'
    )
    parser.add_argument(
        '--repo-root',
        type=Path,
        default=Path.cwd(),
        help='Path to repository root (default: current directory)'
    )
    parser.add_argument(
        '--format',
        choices=['text', 'markdown'],
        default='text',
        help='Output format (default: text)'
    )
    parser.add_argument(
        '--github-output',
        type=Path,
        help='Path to GitHub Actions output file for setting outputs'
    )

    args = parser.parse_args()

    # Run validation
    violations, total_apps = check_all_app_names(args.repo_root)

    # Format and print output
    if args.format == 'markdown':
        output = format_output_markdown(violations, total_apps)
    else:
        output = format_output_text(violations, total_apps)

    print(output)

    # Set GitHub Actions outputs if requested
    if args.github_output:
        with open(args.github_output, 'a') as f:
            f.write(f"violations_count={len(violations)}\n")
            f.write(f"total_apps={total_apps}\n")
            # Escape markdown for multiline output
            markdown_output = format_output_markdown(violations, total_apps)
            # Use heredoc syntax for multiline output
            f.write("comment_body<<EOF\n")
            f.write(markdown_output)
            f.write("\nEOF\n")

    # Exit with error code if violations found
    sys.exit(1 if violations else 0)


if __name__ == '__main__':
    main()
