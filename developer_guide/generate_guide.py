#!/usr/bin/env python3

import argparse
import sys
import subprocess
import shutil
import glob


def main():
    args = parse_args()
    if not check_prereqs():
        sys.exit(-1)

    input_markdown_files = glob.glob("*.md")

    pandoc_pdf_command = ["pandoc", "--pdf-engine", "weasyprint",
                          "-c", "style.css",
                          "--metadata", "title=Magic Lantern Developer Guide",
                          "-s", "-o", "developer_guide.pdf"]

    pandoc_html_command = ["pandoc",
                           "-c", "style.css",
                           "--metadata", "title=Magic Lantern Developer Guide",
                           "-s", "-o", "developer_guide.html"]

    pandoc_pdf_command.extend(input_markdown_files)
    pandoc_html_command.extend(input_markdown_files)

    subprocess.run(pandoc_pdf_command)
    subprocess.run(pandoc_html_command)


def check_prereqs():
    required_commands = ["pandoc",
                         "weasyprint",
                        ]
    for c in required_commands:
        if not shutil.which(c):
            print("ERROR: %s not found" % c)
            return False

    return True


def parse_args():
    description = """Creates a Developer Guide from
    an expected set of Markdown and other files (in this dir).

    Both an HTML and PDF format output should be created.
    """

    parser = argparse.ArgumentParser(description=description)
    args = parser.parse_args()

    return args


if __name__ == "__main__":
    main()
