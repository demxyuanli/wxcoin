#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re

def test_svg_regex():
    # Read pencil.svg
    try:
        with open('pencil.svg', 'r', encoding='utf-8') as f:
            pencil_content = f.read()
    except:
        print("Cannot read pencil.svg")
        return
    
    # Read file.svg
    try:
        with open('file.svg', 'r', encoding='utf-8') as f:
            file_content = f.read()
    except:
        print("Cannot read file.svg")
        return
    
    # Test the regex pattern from SvgIconManager
    path_pattern = r'<path\s+([^>]*?)>'
    
    print("=== Pencil.svg Analysis ===")
    print(f"Content length: {len(pencil_content)}")
    print("Content preview:")
    print(pencil_content[:200])
    print("\nPath regex matches:")
    pencil_matches = re.findall(path_pattern, pencil_content, re.IGNORECASE)
    print(f"Number of matches: {len(pencil_matches)}")
    for i, match in enumerate(pencil_matches):
        print(f"Match {i+1}: {match[:100]}")
    
    print("\n=== File.svg Analysis ===")
    print(f"Content length: {len(file_content)}")
    print("Content preview:")
    print(file_content[:200])
    print("\nPath regex matches:")
    file_matches = re.findall(path_pattern, file_content, re.IGNORECASE)
    print(f"Number of matches: {len(file_matches)}")
    for i, match in enumerate(file_matches):
        print(f"Match {i+1}: {match[:100]}")
    
    # Test if path tags have fill or stroke attributes
    print("\n=== Path Attribute Analysis ===")
    
    def analyze_path_attributes(content, filename):
        print(f"\n{filename}:")
        full_path_pattern = r'<path\s+([^>]*?)>'
        matches = re.findall(full_path_pattern, content, re.IGNORECASE)
        for i, attrs in enumerate(matches):
            has_fill = 'fill=' in attrs.lower()
            has_stroke = 'stroke=' in attrs.lower()
            print(f"  Path {i+1}: fill={has_fill}, stroke={has_stroke}")
            print(f"    Attributes: {attrs[:100]}")
            
            # Check if this path would get default fill
            if not has_fill and not has_stroke:
                print(f"    -> This path WOULD get default fill color")
            else:
                print(f"    -> This path would NOT get default fill color")
    
    analyze_path_attributes(pencil_content, "pencil.svg")
    analyze_path_attributes(file_content, "file.svg")

if __name__ == "__main__":
    test_svg_regex() 