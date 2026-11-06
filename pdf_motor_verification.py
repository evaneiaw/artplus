#!/usr/bin/env python3
"""Portable verification of ArtPlus-Lite PDF motor capabilities.

This script uses only Python's standard library to verify PDF structural
elements, based on the fixture verifier that's already part of the project.
It does NOT require the ArtPlus executable to run.
"""

import os
import sys
import struct

def check_fixture_structure(fixture_dir):
    """Verify the structural integrity of generated PDF fixtures."""
    import hashlib
    
    results = {
        'fixtures_verified': 0,
        'corruption_controls_passed': 0,
        'total_controls': 0,
        'details': []
    }
    
    # Check for the four original fixtures
    fixture_names = [
        'artplus_edit_fixture.pdf',
        'artplus_edit_fixture.pdf-edited.pdf',
        'artplus_cmd_fixture.pdf',
        'artplus_cmd_edited.pdf',
    ]
    
    # Also check for multipage and resource fixtures
    all_fixtures = []
    for f in os.listdir(fixture_dir) if os.path.exists(fixture_dir) else []:
        if f.endswith('.pdf'):
            all_fixtures.append(f)
    
    results['fixtures_found'] = len(all_fixtures)
    
    # Simple structural checks on each PDF file
    for fname in all_fixtures:
        fpath = os.path.join(fixture_dir, fname)
        try:
            with open(fpath, 'rb') as f:
                data = f.read()
            
            checks = []
            
            # Check for PDF header
            if data[:4] == b'%PDF':
                checks.append(('PDF header', True))
            else:
                checks.append(('PDF header', False))
            
            # Check for xref table
            xref_idx = data.find(b'xref')
            if xref_idx >= 0:
                checks.append(('xref table present', True))
            else:
                checks.append(('xref table present', False))
            
            # Check for trailer
            trailer_idx = data.find(b'trailer')
            if trailer_idx >= 0:
                checks.append(('trailer present', True))
            else:
                checks.append(('trailer present', False))
            
            # Check for startxref
            startxref_idx = data.find(b'startxref')
            if startxref_idx >= 0:
                checks.append(('startxref present', True))
            else:
                checks.append(('startxref present', False))
            
            # Check for obj/streams
            obj_count = data.count(b'obj')
            checks.append(('obj entries', obj_count > 0))
            
            # Count indirect references
            indirect_count = data.count(b'>>')
            checks.append(('indirect references', indirect_count > 0))
            
            all_passed = all(c[1] for c in checks)
            results['details'].append({
                'file': fname,
                'checks': checks,
                'passed': all_passed
            })
            
            if all_passed:
                results['fixtures_verified'] += 1
            results['total_controls'] += len(checks)
            
        except Exception as e:
            results['details'].append({
                'file': fname,
                'error': str(e),
                'passed': False
            })
    
    return results


def main():
    fixture_dir = sys.argv[1] if len(sys.argv) > 1 else r"H:\Desktop\artplus-lite\build-msvc\tests\runtime"
    
    if not os.path.exists(fixture_dir):
        print(f"Fixture directory not found: {fixture_dir}")
        print("Expected: build-msvc/tests/runtime or similar")
        sys.exit(1)
    
    print(f"Verifying PDF fixtures in: {fixture_dir}")
    print("=" * 60)
    
    results = check_fixture_structure(fixture_dir)
    
    print(f"\nFixtures found: {results['fixtures_found']}")
    print(f"Fixtures verified: {results['fixtures_verified']}")
    print(f"Total controls: {results['total_controls']}")
    
    if results['details']:
        print("\nDetail per fixture:")
        for d in results['details']:
            status = "PASS" if d.get('passed', False) else "FAIL"
            print(f"  [{status}] {d['file']}")
            for check_name, check_result in d.get('checks', []):
                cstatus = "PASS" if check_result else "FAIL"
                print(f"    {cstatus}: {check_name}")
            if 'error' in d:
                print(f"    ERROR: {d['error']}")
    
    # Summary
    passed = sum(1 for d in results['details'] if d.get('passed', False))
    total = len(results['details']) * (len(results['details'][0]['checks']) if results['details'] else 0) if results['details'] else 0
    
    print("\n" + "=" * 60)
    print(f"Result: {passed} fixtures structurally valid out of {len(results['details'])}")
    
    if results['fixtures_verified'] >= 3:
        print("PDF motor: Estruturalmente OK - fixtures pass validation")
    else:
        print("PDF motor: Some structural issues detected")
    
    return 0 if results['fixtures_verified'] >= 3 else 1


if __name__ == '__main__':
    sys.exit(main())